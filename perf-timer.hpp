/* simple PMU counting capabilities */
#ifndef PERF_TIMER_H_
#define PERF_TIMER_H_

#include <cinttypes>
#include <cstddef>
#include <cstring>
#include <string>
#include <vector>

#define MAX_COUNTERS 8

struct PerfEvent {
    const char *name;
    const char *event_string;

    PerfEvent(const char* name, const char* event_string) : name{name}, event_string{event_string} {}

    bool operator==(const PerfEvent& rhs) const { return std::strcmp(name, rhs.name) == 0; }
    bool operator!=(const PerfEvent& rhs) const { return !(*this == rhs); }
    bool operator< (const PerfEvent& rhs) const { return std::strcmp(name, rhs.name) < 0; }
};

static inline std::string to_string(const PerfEvent& e) {
    return std::string("event[name=") + e.name + ",event_string=" + e.event_string + "]";
}

struct event_counts {
    uint64_t counts[MAX_COUNTERS] = {};

    /** apply binary op to every pair of elements in the array, returning a new event_counts */
    template<typename F>
    static event_counts apply(const event_counts& l, const event_counts& r, const F& f) {
        event_counts ret;
        for (size_t i = 0; i < MAX_COUNTERS; i++) {
            ret.counts[i] = f(l.counts[i], r.counts[i]);
        }
        return ret;
    }
};

void set_verbose(bool verbose);

void list_events();

/**
 * Sets up the PMU to record the given events. Currently doesn't remove
 * any events set up earlier, so the list will keep growing (often you)
 * just set up counters once for the lifetime of the process.
 *
 * Returns a list of indexes, corresponding to the past events: the index
 * is the location in event_counts where you'll find the corresponding
 * counter.
 */
std::vector<bool> setup_counters(const std::vector<PerfEvent>& events);

event_counts read_counters();

/* number of succesfully programmed counters */
size_t num_counters();

event_counts calc_delta(event_counts before, event_counts after);

std::vector<PerfEvent> get_all_events();

#endif // #ifndef PERF_TIMER_H_
