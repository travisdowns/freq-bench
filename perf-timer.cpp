#include "tsc-support.hpp"
#include "perf-timer.hpp"
#include "misc.hpp"
#include "perf-timer-events.hpp"

extern "C" {
#include "jevents/rdpmc.h"
#include "jevents/jevents.h"
}

#include <stdio.h>
#include <string.h>
#include <linux/perf_event.h>
#include <assert.h>

#include <vector>

static bool verbose;
static bool debug; // lots of output

struct event_ctx {
    event_ctx(PerfEvent event, struct perf_event_attr attr, struct rdpmc_ctx jevent_ctx) :
            event{event}, attr{attr}, jevent_ctx{jevent_ctx} {}

    // the associated event
    PerfEvent event;
    // the perf_event_attr structure that was used to open the event
    struct perf_event_attr attr;
    // the jevents context object
    struct rdpmc_ctx jevent_ctx;
};

std::vector<event_ctx> contexts;

/**
 * If true, echo debugging info about the perf timer operation to stderr.
 * Defaults to false.
 */
void set_verbose(bool v) {
    verbose = v;
}

#define vprint(...) do { if (verbose) fprintf(stderr, __VA_ARGS__ ); } while(false)

/**
 * Take a perf_event_attr objects and return a string representation suitable
 * for use as an event for perf, or just for display.
 */
void printf_perf_attr(FILE *f, const struct perf_event_attr* attr) {
    char* pmu = resolve_pmu(attr->type);
    fputs(pmu ? pmu : "???", f);
    bool comma = false;


#define APPEND_IF_NZ1(field) APPEND_IF_NZ2(field,field)
#define APPEND_IF_NZ2(name, field) if (attr->field) { \
        fprintf(f, "/" #name "%s=0x%lx", comma ? "," : "", (long)attr->field); \
        comma = true; \
    }

    APPEND_IF_NZ1(config);
    APPEND_IF_NZ1(config1);
    APPEND_IF_NZ1(config2);
    APPEND_IF_NZ2(period, sample_period);
    APPEND_IF_NZ1(sample_type);
    APPEND_IF_NZ1(read_format);

    fprintf(f, "/");
}

void print_caps(FILE *f, const struct rdpmc_ctx *ctx) {
    fprintf(f, "R%d UT%d ZT%d index: 0x%x",
        (int)ctx->buf->cap_user_rdpmc, (int)ctx->buf->cap_user_time, (int)ctx->buf->cap_user_time_zero, ctx->buf->index);

#define APPEND_CTX_FIELD(field) fprintf(f, " " #field "=0x%lx", (long unsigned)ctx->buf->field);

    APPEND_CTX_FIELD(pmc_width);
    APPEND_CTX_FIELD(offset);
    APPEND_CTX_FIELD(time_enabled);
    APPEND_CTX_FIELD(time_running);

    fprintf(f, " rdtsc=0x%lx", (long unsigned)rdtsc());
}

/* list the events in markdown format */
void list_events() {
    const char *fmt = "| %-27s |\n";
    printf(fmt, "Name");
    printf(fmt, "-------------------------", "-----------");
    for (auto& e : get_all_events()) {
        printf(fmt, e.name);
    }
}

std::vector<bool> setup_counters(const std::vector<PerfEvent>& events) {

    std::vector<bool> results;

    for (auto& e : events) {
        bool ok = false;

        if (contexts.size() == MAX_COUNTERS) {
            fprintf(stderr, "Unable to program event %s, MAX_COUNTERS (%zu) reached\n", e.name, MAX_COUNTERS);
        } else {
            // fprintf(stderr, "Enabling event %s (%s)\n", e->short_name, e->name);
            struct perf_event_attr attr = {};
            int err = jevent_name_to_attr(e.event_string, &attr);
            if (err) {
                fprintf(stderr, "Unable to resolve event '%s' - report this as a bug along with your CPU model string\n", e.name);
                fprintf(stderr, "jevents error %2d: %s\n", err, jevent_error_to_string(err));
                fprintf(stderr, "jevents details : %s\n", jevent_get_error_details());
            } else {
                struct rdpmc_ctx ctx = {};
                attr.sample_period = 0;
                // pinned makes the counter stay on the CPU and fail fast if it can't be allocated: we
                // can check right away if index == 0 which means failure
                attr.pinned = 1;
                // attr.read_format = PERF_FORMAT_TOTAL_TIME_ENABLED | PERF_FORMAT_TOTAL_TIME_RUNNING;
                int ret;
                if ((ret = rdpmc_open_attr(&attr, &ctx, 0)) || ctx.buf->index == 0) {
                    fprintf(stderr, "Failed to program event '%s' (reason: %s). \n\tResolved to: ", e.name,
                            ret ? "rdpmc_open_attr failed" : "no index, probably too many or incompatible events");
                    printf_perf_attr(stderr, &attr);
                    fprintf(stderr, "\n");
                } else {
                    contexts.emplace_back(e, attr, ctx);
                    ok = true;
                }
            }
        }
        results.push_back(ok);
    }

    // output all the event details after all have been programmed since later events with constaints might
    // change the index for earlier ones
    if (verbose) {
        for (size_t i = 0; i < contexts.size(); i++) {
            event_ctx ec = contexts[i];
            vprint("Resolved and programmed event '%s' to ", ec.event.name);
            printf_perf_attr(stderr, &ec.attr);
            vprint("\n\t");
            print_caps(stderr, &ec.jevent_ctx);
            vprint("\n");
        }
    }

    assert(results.size() == events.size());
    return results;
}

/**
 * rdpmc_read - read a ring 3 readable performance counter
 * @ctx: Pointer to initialized &rdpmc_ctx structure.
 *
 * Read the current value of a running performance counter.
 * This should only be called from the same thread/process as opened
 * the context. For new threads please create a new context.
 */
unsigned long long rdpmc_readx(event_ctx *ctx)
{
    typedef uint64_t u64;
#define rmb() asm volatile("" ::: "memory")

	u64 val;
	unsigned seq;
	u64 offset, time_running, time_enabled;
	struct perf_event_mmap_page *buf = ctx->jevent_ctx.buf;
	unsigned index;
    bool lockok = true;

	do {
		seq = buf->lock;
		rmb();
		index = buf->index;
		offset = buf->offset;
        time_enabled = buf->time_enabled;
        time_running = buf->time_running;
		if (index == 0) { /* rdpmc not allowed */
            val = 0;
            rmb();
            lockok = (buf->lock == seq);
			break;
        }
#if defined(__ICC) || defined(__INTEL_COMPILER)
		val = _rdpmc(index - 1);
#else
		val = __builtin_ia32_rdpmc(index - 1);
#endif
		rmb();
	} while (buf->lock != seq);

    u64 res  = val + offset;
    u64 res2 = (res << (64 - buf->pmc_width)) >> (64 - buf->pmc_width);

    if (debug) {
        vprint("read counter %-30s ", ctx->event.name);
#define APPEND_LOCAL(local, fmt) fprintf(stderr, " " #local "=0x%" #fmt "lx", (long unsigned)local);
        APPEND_LOCAL(lockok, 1);
        APPEND_LOCAL(val, 013);
        APPEND_LOCAL(offset, 013);
        // APPEND_LOCAL(res, 013);
        APPEND_LOCAL(res2, 012);
        APPEND_LOCAL(time_enabled, 08);
        APPEND_LOCAL(time_running, 08);
        APPEND_LOCAL(index, );
        vprint("\n");
    }
    return res2;
}


event_counts read_counters() {
    event_counts ret = {};
    for (size_t i = 0; i < contexts.size(); i++) {
        ret.counts[i] = rdpmc_readx(&contexts[i]);
    }
    return ret;
}

size_t num_counters() {
    return contexts.size();
}

event_counts calc_delta(event_counts before, event_counts after, size_t max_event) {
    event_counts ret(uninit_tag{});
    size_t limit = std::min(max_event, MAX_COUNTERS);
    for (size_t i=0; i < limit; i++) {
        ret.counts[i] = after.counts[i] - before.counts[i];
    }
    return ret;
}
