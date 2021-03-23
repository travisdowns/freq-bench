
#include <assert.h>
#include "common-cxx.hpp"
#include "env.hpp"
#include "impl-list.hpp"
#include "misc.hpp"
#include "msr-access.h"
#include "opt-control.h"
#include "pcg-cpp/pcg_random.hpp"
#include "perf-timer-events.hpp"
#include "perf-timer.hpp"
#include "tsc-support.hpp"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <map>

#include <math.h>
#include <sys/mman.h>
#include <time.h>

#include <emmintrin.h>
#include <immintrin.h>

#include <sched.h>

// #include "dbg.h"

using namespace env;

static bool verbose;
static bool summary;
static bool debug;
static bool prefix_cols;
static bool no_warm;  // true == skip the warmup stamp() each iteration

static uint64_t tsc_freq;

using velem = std::vector<char>;

#define vprint(...)                       \
    do {                                  \
        if (verbose)                      \
            fprintf(stderr, __VA_ARGS__); \
    } while (false)
#define dprint(...)                       \
    do {                                  \
        if (debug)                        \
            fprintf(stderr, __VA_ARGS__); \
    } while (false)

void flushmem(const void* p, size_t size) {
    for (size_t i = 0; i < size; i += 64) {
        _mm_clflush((char*)p + i);
    }
    _mm_mfence();
}

void pinToCpu(int cpu) {
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(cpu, &set);
    if (sched_setaffinity(0, sizeof(set), &set)) {
        assert("pinning failed" && false);
    }
}

template <typename... Args>
void usageCheck(bool condition, const std::string& fmt, Args... args) {
    if (!condition) {
        fprintf(stderr,
                "%s\n"
                "Usage:\n"
                "\tbench [TEST_NAME]\n"
                "\n where you can optionally provide TEST_NAME to run a specific test:\n\n",
                string_format(fmt, args...).c_str());

        for (auto& desc : get_all()) {
            fprintf(stderr, " %s\n\t%s\n", desc.name, desc.desc);
        }
        exit(EXIT_FAILURE);
    }
}

/* dump the tests in a single space-separate strings, perhaps convenient so you can do something like:
 *     for test in $(DUMPTESTS=1 ./bench); do ./bench $test; done
 * to run all tests. */
void dump_tests() {
    for (auto& desc : get_all()) {
        printf("%s ", desc.name);
    }
}

/* return the alignment of the given pointer
   i.e., the largest power of two that divides it */
size_t get_alignment(const void* p) {
    return (size_t)((1UL << __builtin_ctzl((uintptr_t)p)));
}

/**
 * Get the relative alignment of two pointers. The relative alignment is determined
 * by the number of equal least significant bits.
 */
size_t relalign(const void* a, const void* b) {
    return get_alignment((void*)((uintptr_t)a ^ (uintptr_t)b));
}

void update_min_counts(bool first, event_counts* min, event_counts cur) {
    for (size_t i = 0; i < MAX_COUNTERS; i++) {
        min->counts[i] = (first || cur.counts[i] < min->counts[i] ? cur.counts[i] : min->counts[i]);
    }
}

struct iprinter;

struct RunArgs {
    double busy;
    size_t repeat_count, iters;

    /**
     * Get the intersect args appropriate for the given iteration.
     */
    bench_args get_args() const { return {}; }
};

class StampConfig;

/**
 * Test stamp encapsulates everything we measure before and after the code
 * being benchmarked.
 */
class Stamp {
    friend StampConfig;

public:
    constexpr static size_t  MAX_MSR = 1;

    Stamp() : tsc(-1) {}

    Stamp(uint64_t tsc, event_counts counters, uint64_t tsc_before, size_t retries)
        : tsc{tsc},  tsc_before{tsc_before}, counters{counters}, retries{retries}, msrs_read{0} {}

    std::string to_string() { return std::string("tsc: ") + std::to_string(this->tsc); }

    uint64_t tsc, tsc_before;
    event_counts counters;
    size_t retries;
    uint64_t msr_values[MAX_MSR];
    size_t msrs_read;
};

class StampConfig;

/**
 * Thrown when the caller asks for a counter that was never configured.
 */
struct NonExistentCounter : public std::logic_error {
    NonExistentCounter(const PerfEvent& e) : std::logic_error(std::string("counter ") + e.name + " doesn't exist") {}
};

/**
 * Taking the delta of two stamps gives you a stamp delta.
 *
 * The StampData has a reference to the StampConfig from which it was created, so the
 * lifetime of the StampConfig must be at least as long as the StampDelta.
 */
class StampDelta {
    friend Stamp;
    friend StampConfig;

    bool empty;
    const StampConfig* config;
    // not cycles: has arbitrary units
    uint64_t tsc_delta;
    event_counts counters;

    StampDelta(const StampConfig& config,
               uint64_t tsc_delta,
               event_counts counters)
        : empty(false),
          config{&config},
          tsc_delta{tsc_delta},
          counters{std::move(counters)}
          {}

public:
    /**
     * Create an "empty" delta - the only thing an empty delta does is
     * never be returned from functions like min(), unless both arguments
     * are empty. Handy for accumulation patterns.
     */
    StampDelta() : empty(true), config{nullptr}, tsc_delta{}, counters{} {}

    double get_nanos() const {
        assert(!empty);
        return 1000000000. * tsc_delta / tsc_freq;
    }

    uint64_t get_tsc() const {
        assert(!empty);
        return tsc_delta;
    }

    event_counts get_counters() const {
        assert(!empty);
        return counters;
    }

    const StampConfig& get_config() const {
        return *config;
    }

    uint64_t get_counter(const PerfEvent& event) const;

    /**
     * Return a new StampDelta with every contained element having the minimum
     * value between the left and right arguments.
     *
     * As a special rule, if either argument is empty, the other argument is returned
     * without applying the function, this facilitates typical use with an initial
     * empty object followed by accumulation.
     */
    template <typename F>
    static StampDelta apply(const StampDelta& l, const StampDelta& r, F f) {
        if (l.empty)
            return r;
        if (r.empty)
            return l;
        assert(l.config == r.config);
        event_counts new_counts            = event_counts::apply(l.counters, r.counters, f);
        return StampDelta{*l.config, {f(l.tsc_delta, r.tsc_delta)}, new_counts};
    }

    static StampDelta min(const StampDelta& l, const StampDelta& r) { return apply(l, r, min_functor{}); }
    static StampDelta max(const StampDelta& l, const StampDelta& r) { return apply(l, r, max_functor{}); }
};

const PerfEvent DUMMY_EVENT_NANOS = PerfEvent("nanos", "nanos");

/**
 * Manages PMU events.
 */
class EventManager {
    /** event to counter index */
    std::map<PerfEvent, size_t> event_map;
    std::vector<PerfEvent> event_vec;
    std::vector<bool> setup_results;
    size_t next_counter;
    bool prepared;

public:
    EventManager() : next_counter(0), prepared(false) {}

    bool add_event(const PerfEvent& event) {
        if (event == NoEvent || event == DUMMY_EVENT_NANOS) {
            return true;
        }
        vprint("Adding event %s\n", to_string(event).c_str());
        prepared = false;
        if (event_map.count(event)) {
            return true;
        }
        if (event_map.size() == MAX_COUNTERS) {
            return false;
        }
        event_map.insert({event, next_counter++});
        event_vec.push_back(event);
        return true;
    }

    void prepare() {
        assert(event_map.size() == event_vec.size());
        setup_results   = setup_counters(event_vec);
        size_t failures = std::count(setup_results.begin(), setup_results.end(), false);
        if (failures > 0) {
            fprintf(stderr, "%zu events failed to be configured\n", failures);
        }
        vprint("EventManager configured %zu events\n", (setup_results.size() - failures));
        prepared = true;
    }

    /**
     * Return the counter slot for the given event, or -1
     * if the event was requested but setting up the counter
     * failed.
     *
     * If an event is requested that was never configured,
     * NonExistentCounter exception is thrown.
     */
    ssize_t get_mapping(const PerfEvent& event) const {
        if (!prepared) {
            throw std::logic_error("not prepared");
        }
        auto mapit = event_map.find(event);
        if (mapit == event_map.end()) {
            throw NonExistentCounter(event);
        }
        size_t idx = mapit->second;
        if (!setup_results.at(idx)) {
            return -1;
        }
        return idx;
    }

    /** number of unique configured events */
    size_t get_count() {
        return event_map.size();
    }
};

/**
 * Manages PMU events.
 */
class MSRManager {

    std::vector<uint32_t> msrids;

    using result_type = uint64_t[Stamp::MAX_MSR];

    uint64_t read_msr(uint32_t id) const {
        uint64_t value = 0;
        int err = read_msr_cur_cpu(id, &value);
        (void)err;
        assert(err == 0);
        return value;
    }

public:
    MSRManager() {}

    void add_msr(uint32_t id) {
        msrids.push_back(id);
    }

    void prepare() {
        if (msrids.size() > Stamp::MAX_MSR) {
            throw std::runtime_error("number of MSR reads exceeds MAX_MSR"); // just increase MAX_MSR
        }
        // try to read all the configured MSRs, in order to fail fast
        for (auto id : msrids) {
            uint64_t value = 0;
            int err = read_msr_cur_cpu(id, &value);
            if (err) {
                throw std::runtime_error(std::string("MSR ") + std::to_string(id) + " read failed with error " + std::to_string(err));
            }
        }
    }

    HEDLEY_ALWAYS_INLINE
    void do_stamp(Stamp &stamp) const {
        if (HEDLEY_UNLIKELY(!msrids.empty())) {
            do_stamp_slowpath(stamp);
        }
    }

    HEDLEY_NEVER_INLINE
    void do_stamp_slowpath(Stamp &stamp) const {
        size_t i = 0;
        for (auto id : msrids) {
            stamp.msr_values[i++] = read_msr(id);
        }
        stamp.msrs_read = i;
        // dbg(stamp.msrs_read);
    }

    uint64_t get_value(uint32_t id, const Stamp& stamp) const {
        auto pos = std::find(msrids.begin(), msrids.end(), id);
        // dbg(msrids.size());
        if (pos == msrids.end()) {
            throw std::logic_error("MSR id not found in list");
        }
        size_t idx = pos - msrids.begin();
        if (idx >= stamp.msrs_read) {
            // dbg(idx);
            // dbg(stamp.msrs_read);
            throw std::logic_error("MSR wasnt read");
        }
        assert(idx < Stamp::MAX_MSR);
        return stamp.msr_values[idx];
    }
};

/**
 * A class that holds configuration for creating stamps.
 *
 * Configured based on what columns are requested, holds configuration for varous types
 * of objects.
 */
class StampConfig {
public:
    constexpr static size_t MAX_RETRIES = 10;

    EventManager em;
    MSRManager mm;
    uint64_t retry_gap;

    StampConfig () : retry_gap{-1u} {}

    /**
     * After updating the config to the state you want, call prepare() once which
     * does any global configuration needed to support the configured stamps, such
     * as programming PMU events.
     */
    void prepare() {
        em.prepare();
        mm.prepare();
        // dirty hack - estimate retry gap based on number of events and magic
        // numbers
        retry_gap = 30 + 50 * em.get_count();
    }

    // take the stamp.
    Stamp stamp() const {
        auto tsc_before = rdtsc();
        auto counters = read_counters();
        auto tsc = rdtsc();

        Stamp s(tsc, counters, tsc_before, 0);
        mm.do_stamp(s);

        if (HEDLEY_LIKELY(tsc - tsc_before <= retry_gap)) {
            return s;
        }

        size_t retries = 1;
        do {
            tsc_before = rdtsc();
            counters = read_counters();
            tsc = rdtsc();
        } while (tsc - tsc_before > retry_gap && retries++ < MAX_RETRIES);

        s = {tsc, counters, tsc_before, retries};
        mm.do_stamp(s);

        return s;
    }

    /**
     * Create a StampDelta from the given before/after stamps
     * which should have been created by this StampConfig.
     */
    StampDelta delta(const Stamp& before, const Stamp& after) const {
        return StampDelta(*this, after.tsc - before.tsc, calc_delta(before.counters, after.counters));
    }
};

uint64_t StampDelta::get_counter(const PerfEvent& event) const {
    const EventManager& em = config->em;
    ssize_t idx            = em.get_mapping(event);
    assert(idx >= -1 && idx <= (ssize_t)MAX_COUNTERS);
    if (idx == -1) {
        return -1;
    }
    return this->counters.counts[idx];
}

struct BenchResults {
    StampDelta delta;
    Stamp after;
    RunArgs args;
    uint64_t start_tsc;

    BenchResults() = delete;
};

/** thrown by get_value if a column failed for some reason */
struct ColFailed : std::runtime_error {
    /* colval_ is used as the column value */
    std::string colval_;
    ColFailed(std::string colval) : std::runtime_error("column failed"), colval_{std::move(colval)} {}
};

/**
 * A Column object represents a thing which knows how to print a column of data.
 * It injects what it wants into the Stamp object and then gets it back out after.
 *
 *
 */
class Column {
public:

private:
    const char* heading;
    const char* format;
    bool post_output;

protected:
    /* subclasses implement this to return the value associated with the metric */
    virtual std::pair<double, bool> get_value(const BenchResults& results) const {
        throw std::logic_error("unimplemented get_value");
    };

public:
    Column(const char* heading, const char* format = nullptr, bool post_output = false)
        : heading{heading}, format{format}, post_output{post_output} {}

    virtual ~Column() {}

    virtual const char* get_header() const { return heading; }
    virtual int col_width() const { return std::max(4, (int)strlen(heading)) + 1; }

    /* subclasses can implement this to modify the StampConfig as needed in order to get the values needed
       for this column */
    virtual void update_config(StampConfig& sc) const {}

    /**
     * "post output" columns don't get get printed in the table like
     * other columns but rather are output one at a time after each
     * repeat, since they output a lot of data
     */
    virtual bool is_post_output() const { return post_output; }

    double get_final_value(const BenchResults& results) const {
        auto val = get_value(results);
        if (val.second) {
            return val.first;
        } else {
            return std::numeric_limits<double>::quiet_NaN();
        }
    }

    virtual void print(FILE* f, const BenchResults& results) const { print(f, get_final_value(results)); }

    void print(FILE* f, double val) const { print(f, formatted_string(val)); }

    void print(FILE* f, const std::string& s) const {
        auto formatted = string_format("%*s", col_width(), s.c_str());
        if ((int)formatted.size() > col_width())
            formatted.resize(col_width());
        fprintf(f, "%s", formatted.c_str());
    }

    /** return the string representation of the results */
    virtual std::string formatted_string(double val) const {
        try {
            if (std::isnan(val)) {
                return string_format("%*s", col_width(), "-");
            } else {
                return string_format(format, col_width(), val);
            }
        } catch (ColFailed& fail) {
            auto ret = string_format("%*s", col_width(), fail.colval_.c_str());
            if ((int)ret.size() > col_width())
                ret.resize(col_width());
            return ret;
        }
    }
};

class EventColumn : public Column {
public:
    PerfEvent top, bottom;

    EventColumn(const char* heading, const char* format, PerfEvent top, PerfEvent bottom)
        : Column{heading, format}, top{top}, bottom{bottom} {}

    virtual std::pair<double, bool> get_value(const BenchResults& results) const override {
        double ratio = value(results.delta, top) / (is_ratio() ? value(results.delta, bottom) : 1.);
        return {ratio, true};
    }

    void update_config(StampConfig& sc) const override {
        sc.em.add_event(top);
        sc.em.add_event(bottom);
    }

    /** true if this value is a ratio (no need to normalize), false otherwise */
    bool is_ratio() const {
        return bottom != NoEvent;  // lol
    }

private:
    double value(const StampDelta& delta, const PerfEvent& e) const {
        if (e == DUMMY_EVENT_NANOS) {
            return delta.get_nanos();
        }
        auto v = delta.get_counter(e);
        if (v == (uint64_t)-1) {
            throw ColFailed("fail");
        }
        return v;
    }
};

EventColumn EVENT_COLUMNS[] = {

        {"INSTRU", "%*.2f", INST_RETIRED_ANY, NoEvent},
        {"IPC", "%*.2f", INST_RETIRED_ANY, CPU_CLK_UNHALTED_THREAD},
        {"UPC", "%*.2f", UOPS_ISSUED_ANY, CPU_CLK_UNHALTED_THREAD},
        {"UOPS", "%*.2f", UOPS_ISSUED_ANY, NoEvent},
        {"MLP1A", "%*.2f", L1D_PEND_MISS_PENDING, CPU_CLK_UNHALTED_THREAD},
        {"MLP1B", "%*.2f", L1D_PEND_MISS_PENDING, L1D_PEND_MISS_PENDING_CYCLES},
        {"LAT", "%*.0f", L1D_PEND_MISS_PENDING, MEM_LOAD_RETIRED_L1_MISS},
        {"LOADO %", "%*.2f", L1D_PEND_MISS_PENDING_CYCLES, CPU_CLK_UNHALTED_THREAD},
        {"ALL_LOAD", "%*.2f", MEM_INST_RETIRED_ALL_LOADS, NoEvent},
        {"L1_MISS", "%*.1f", MEM_LOAD_RETIRED_L1_MISS, NoEvent},
        {"L1_REPL", "%*.1f", L1D_REPLACEMENT, NoEvent},

        {"Cycles", "%*.2f", CPU_CLK_UNHALTED_THREAD, NoEvent},
        {"Unhalt_GHz", "%*.3f", CPU_CLK_UNHALTED_THREAD, DUMMY_EVENT_NANOS},
        // {"Unhalt_GHz", "%*.3f", CPU_CLK_UNHALTED_REF_TSC, DUMMY_EVENT_NANOS},

        {"P0", "%*.2f", UOPS_DISPATCHED_PORT_PORT_0, CPU_CLK_UNHALTED_THREAD},
        {"P1", "%*.2f", UOPS_DISPATCHED_PORT_PORT_1, CPU_CLK_UNHALTED_THREAD},
        {"P2", "%*.2f", UOPS_DISPATCHED_PORT_PORT_2, CPU_CLK_UNHALTED_THREAD},
        {"P3", "%*.2f", UOPS_DISPATCHED_PORT_PORT_3, CPU_CLK_UNHALTED_THREAD},
        {"P4", "%*.2f", UOPS_DISPATCHED_PORT_PORT_4, CPU_CLK_UNHALTED_THREAD},
        {"P5", "%*.2f", UOPS_DISPATCHED_PORT_PORT_5, CPU_CLK_UNHALTED_THREAD},
        {"P6", "%*.2f", UOPS_DISPATCHED_PORT_PORT_6, CPU_CLK_UNHALTED_THREAD},
        {"P7", "%*.2f", UOPS_DISPATCHED_PORT_PORT_7, CPU_CLK_UNHALTED_THREAD},

};

/**
 * The simplest column just lets you specify an "extractor" function to return the value given a
 * BenchResults object.
 */
class SimpleColumn : public Column {
public:
    using extractor_fn = std::function<double(const BenchResults& ir)>;
    extractor_fn extractor;

    SimpleColumn(const char* heading, extractor_fn extractor)
        : Column{heading}, extractor{extractor} {}

    virtual std::pair<double, bool> get_value(const BenchResults& results) const override {
        return {extractor(results), true};
    }
};

using BR = const BenchResults&;

SimpleColumn BASIC_COLUMNS[] = {
        {"tsc-delta",  [](BR r) {return (double)r.delta.get_tsc(); }},
        {"tscb",    [](BR r) { return (double)r.after.tsc_before - r.start_tsc; }}, // tsc before read_counters
        {"tsca",    [](BR r) { return (double)r.after.tsc - r.start_tsc; }},        // tsc after  read_counters
        {"tscg",    [](BR r) { return (double)r.after.tsc - r.after.tsc_before; }}, // tscb/a gap
        {"retries", [](BR r) { return (double)r.after.retries; }},                  // how many times the counters were re-read
        {"nanos",   [](BR r) { return r.delta.get_nanos(); }}
};

template <class T>
T extract_bits(T val, size_t start, size_t stop) {
    assert(stop >= start);
    assert(start <= sizeof(T) * 8);
    T width = stop - start + 1;
    if (width == sizeof(T) * 8) {
        return val;
    } else {
        return (val >> (T)start) & ((1 << width) - 1);
    }
}

struct MSRColumn : public Column {
    uint32_t id; // the address/id of the MSR
    size_t startbit, stopbit;
    double scale_by;

    MSRColumn(const char* heading, uint32_t id, size_t startbit = 0, size_t stopbit = 63, double scale_by = 1.0)
        : Column{heading}, id{id}, startbit{startbit}, stopbit{stopbit}, scale_by{scale_by} {}

    void update_config(StampConfig& sc) const override {
        sc.mm.add_msr(id);
    }

    virtual std::pair<double, bool> get_value(const BenchResults& results) const override {
        const MSRManager& manager = results.delta.get_config().mm;
        uint64_t value = manager.get_value(id, results.after);
        value = extract_bits(value, startbit, stopbit);
        return { value * scale_by, true };
    }
};

MSRColumn MSR_COLUMNS[] = {
    {"volts", 0x198, 32, 47, 1. / 8192.}
};

using ColList = std::vector<Column*>;

/**
 * Get all the available columns
 */
ColList get_all_columns() {
    ColList ret;
    auto add = [&ret](auto& container) {
        for (auto& c : container) {
            ret.push_back(&c);
        }
    };
    add(BASIC_COLUMNS);
    add(EVENT_COLUMNS);
    add(MSR_COLUMNS);
    return ret;
}

void hot_wait(size_t cycles) {
    volatile int x = 0;
    (void)x;
    while (cycles--) {
        x = 1;
    }
}

size_t test_cycles;
size_t period_cycles;
size_t resolution_cycles;
size_t payload_extra_cycles;

void runOne(const test_description* test,
            const StampConfig& config,
            const ColList& columns,
            const ColList& post_columns,
            const RunArgs& bargs) {

    /* the main benchmark loop */
    std::vector<BenchResults> result_vector;

    auto args = bargs.get_args();

    struct Sample {
        uint64_t tsc, period, sdeadline;
        uint64_t payload_spins, total_spins;
        uint64_t payload_start_tsc, payload_end_tsc;
        Stamp stamp;
    };

    struct RunResult {
        std::vector<Sample> samples;
        uint64_t start_tsc;
        RunResult(size_t sample_count) : samples(sample_count) {}
    };

    std::vector<RunResult> allresults;
    allresults.reserve(bargs.repeat_count);

    for (size_t repeat = 0; repeat < bargs.repeat_count; repeat++) {

        const size_t samples_max = test_cycles / resolution_cycles + 2;
        allresults.emplace_back(samples_max);
        auto& stamps = allresults.back().samples;

        if (!(test->flags & NO_VZ)) {
            _mm256_zeroupper();
        }
        hot_wait(1000000000ull);

        config.stamp();  // warm
        uint64_t tsc = rdtsc(), sample_deadline = tsc, period_deadline = tsc;
        size_t rpos = 0, period = 0;
        allresults.back().start_tsc = tsc;

        while (rpos < samples_max) {
            bool first = true;
            auto payload_deadline = period_deadline + payload_extra_cycles;

            period_deadline += period_cycles;
            while (tsc < period_deadline && rpos < samples_max) {
                sample_deadline += resolution_cycles;
                uint64_t total_spins = 0, payload_spins = 0, payload_start_tsc = rdtsc(), payload_end_tsc = 0;
                do {
                    // while waiting to take a sample we either execute the
                    // busy wait
                    if (first || tsc < payload_deadline) {
                        _mm_lfence();
                        test->call_f(args);
                        payload_spins++;
                        tsc = payload_end_tsc = rdtsc();
                        first = false;
                    } else {
                        tsc = rdtsc();
                    }
                    total_spins++;
                } while (tsc < sample_deadline);

                if (!no_warm) config.stamp();  // warming, reduces outliers
                stamps[rpos++] = {tsc, period, sample_deadline, payload_spins, total_spins,
                        payload_start_tsc, payload_end_tsc, config.stamp()};
            }

            period++;
        }
    }



    for (size_t repeat = 0; repeat < bargs.repeat_count; repeat++) {
        printf("repeat,us,period,sdl,payspin,totspin,paytime");
        for (auto col : columns) {
            if (prefix_cols) {
                printf(",%s %s", test->name, col->get_header());
            } else {
                printf(",%s", col->get_header());
            }
        }
        printf("\n");

        const auto& results = allresults.at(repeat);
        const auto& samples = results.samples;

        for (size_t i = 1; i < samples.size(); i++) {
            const auto& result = samples.at(i);
            StampDelta delta = config.delta(samples.at(i - 1).stamp, result.stamp);

            printf("%zu,%.3f,%zu,%zu,%zu,%zu,%zu", repeat, 1000000. * (result.tsc - results.start_tsc) / tsc_freq,
                    result.period, result.sdeadline - results.start_tsc, result.payload_spins, result.total_spins,
                    result.payload_spins ? (result.payload_end_tsc  - result.payload_start_tsc) / result.payload_spins : 0);
            BenchResults br{delta, result.stamp, bargs, results.start_tsc};
            for (auto column : columns) {
                double val = column->get_final_value(br);
                // printf("\nvalue for %s %f\n", column->get_header(), val);
                ssize_t ival = val;
                if ((double)ival == val) {
                    // integer value
                    printf(",%zd", ival);
                } else {
                    printf(",%.3f", val);
                }
            }
            printf("\n");
        }
    }


}

int main(int argc, char** argv) {
    summary     = getenv_bool("SUMMARY");
    verbose     = !getenv_bool("QUIET");
    debug       = getenv_bool("DEBUG");
    prefix_cols = getenv_bool("PREFIX_COLS");
    no_warm     = getenv_bool("NO_WARM");

    bool dump_tests_flag = getenv_bool("DUMPTESTS");
    bool do_list_events  = getenv_bool("LIST_EVENTS");  // list the events and quit
    bool include_slow    = getenv_bool("INCLUDE_SLOW");
    std::string collist  = getenv_generic<std::string>(
            "COLS", "tsc-delta,nanos,Cycles,INSTRU,IPC,UPC,Unhalt_GHz");

    constexpr size_t SIZE_INC_DEFAULT = 256 * 1024;
    constexpr size_t SIZE_STOP_DEFAULT = 20 * 1024 * 1024;
    size_t size_inc    = getenv_int("INC", SIZE_INC_DEFAULT);
    size_t size_start  = getenv_int("START",  size_inc);
    size_t size_stop   = getenv_int("STOP",  SIZE_STOP_DEFAULT);
    int pincpu         = getenv_int("PINCPU",  0);
    size_t iters       = getenv_int("ITERS", 100);

    test_cycles          = getenv_longlong("TEST_CYC",   1ull *  100ull * 1000ull * 1000ull);
    period_cycles        = getenv_longlong("TEST_PER",            10ull * 1000ull * 1000ull);
    resolution_cycles    = getenv_longlong("TEST_RES",                      10ull * 1000ull);
    payload_extra_cycles = getenv_longlong("TEST_EXTRA",                                  0);

    // size

    if (size_inc != SIZE_INC_DEFAULT || size_stop != SIZE_STOP_DEFAULT) {
        throw std::runtime_error("adjusting the size stop/stop/inc is not supported ");
    }

    assert(iters > 0);

    if (verbose)
        set_verbose(true);  // set perf-timer to verbose too

    if (dump_tests_flag) {
        dump_tests();
        exit(EXIT_SUCCESS);
    }

    if (do_list_events) {
        list_events();
        exit(EXIT_SUCCESS);
    }

    usageCheck(argc == 1 || argc == 2, "Must provide 0 or 1 arguments");

    std::vector<test_description> tests;

    if (argc > 1) {
        tests = get_by_list(argv[1]);
    } else {
        // all tests
        for (auto t : get_all()) {
            if ((include_slow || !(t.flags & SLOW))) {
                tests.push_back(t);
            }
        }
    }

    pinToCpu(pincpu);

    ColList allcolumns, columns, post_columns;
    for (auto requested : split(collist, ",")) {
        if (requested.empty()) {
            continue;
        }
        bool found = false;
        for (auto& col : get_all_columns()) {
            if (requested == col->get_header()) {
                allcolumns.push_back(col);
                found = true;
                break;
            }
        }
        usageCheck(found, "No column named %s", requested.c_str());
    }

    std::copy_if(allcolumns.begin(), allcolumns.end(), std::back_inserter(columns),
                 [](auto& c) { return !c->is_post_output(); });
    std::copy_if(allcolumns.begin(), allcolumns.end(), std::back_inserter(post_columns),
                 [](auto& c) { return c->is_post_output(); });

    vprint("Found %zu normal columns and %zu post-output columns\n", columns.size(), post_columns.size());

    StampConfig config;
    // we give each column a chance to update the StampConfig with what it needs
    for (auto& col : allcolumns) {
        col->update_config(config);
    }
    config.prepare();

    // run the whole test repeat_count times, each of which calls the test function iters times
    unsigned repeat_count = 3;

    bool freq_forced = true;
    tsc_freq = getenv_generic<double>("MHZ", 0.0) * 1000000;
    if (tsc_freq == 0.0) {
        tsc_freq = get_tsc_freq(false);
        freq_forced = false;
    }

    if (verbose) {
        fprintf(stderr, "inner loops  : %10zu\n", iters);
        fprintf(stderr, "pinned cpu   : %10d\n", pincpu);
        fprintf(stderr, "current cpu  : %10d\n", sched_getcpu());
        fprintf(stderr, "start size   : %10zu bytes\n", size_start);
        fprintf(stderr, "stop size    : %10zu bytes\n", size_stop);
        fprintf(stderr, "inc size     : %10zu bytes\n", size_inc);
        fprintf(stderr, "tsc freq     : %10.1f MHz%s\n", tsc_freq / 1000000., freq_forced ? " (forced)" : "");
        fprintf(stderr, "test period  : %10.3f us\n", 1000000. * test_cycles       / tsc_freq);
        fprintf(stderr, "duty period  : %10.3f us\n", 1000000. * period_cycles     / tsc_freq);
        fprintf(stderr, "resolution   : %10.3f us\n", 1000000. * resolution_cycles / tsc_freq);
        fprintf(stderr, "payload extra: %10.3f us\n", 1000000. * payload_extra_cycles / tsc_freq);
        fprintf(stderr, "warmup stamp : %10s\n", no_warm ? "no" : "yes");
    }

    if (!summary) {
        fprintf(stderr, "About to run %zu tests with %zu columns (after %zu ms of startup time)\n", tests.size(),
                columns.size(), (size_t)clock() * 1000u / CLOCKS_PER_SEC);
    }

    RunArgs args{0., repeat_count, iters};
    for (auto t : tests) {
        runOne(&t, config, columns, post_columns, args);
    }

    fprintf(stderr, "Benchmark done\n");
    fflush(stderr);
}
