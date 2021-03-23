#include <stdlib.h>
#include <stdio.h>
#include <sched.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include <err.h>

#include "msr-access.h"

struct result {
    uint64_t value;
    int error;
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

result read_voltage(int cpu) {
    uint64_t value = 0;
    int err = cpu == -1 ? read_msr_cur_cpu(0x198, &value) : read_msr(cpu, 0x198, &value);
    return err ? result{0, err} : result{extract_bits(value, 32, 47), 0};
}

void bench(int iters, bool thiscpu) {
    int i = iters;
    clock_t start = clock();
    while (i-- > 0) {
        result r = read_voltage(thiscpu ? -1 : 0);
        if (r.error) {
            err(1, "call failed in bench with %d", r.error);
        }
    }
    clock_t stop = clock();
    double seconds = (double)(stop - start) / CLOCKS_PER_SEC;
    printf("(%4s CPU) %d calls in %5.2f ms: %5.2f us per call\n",
            thiscpu ? "same" : "other", iters, seconds * 1000., seconds * 1000000. / iters);
}

void pinToCpu(int cpu) {
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(cpu, &set);
    if (sched_setaffinity(0, sizeof(set), &set)) {
        assert("pinning failed" && false);
    }
}

int main(int argc, char** argv) {

    pinToCpu(1);

    if (argc >= 2 && strcmp("--bench", argv[1]) == 0) {
        int iters = (argc == 3 ? atoi(argv[2]) : 10000);
        printf("Running benchmarks with %d iterations\n", iters);
        bench(iters, false);
        bench(iters, false);
        bench(iters, true);
        bench(iters, true);
        bench(iters, false);
        bench(iters, false);
        bench(iters, true);
        bench(iters, true);
        exit(0);
    }

    int maxcpu = 0;
    for (; maxcpu < CPU_SETSIZE; maxcpu++) {
        result r = read_voltage(maxcpu);
        if (r.error)
            break;
    }

    if (maxcpu == 0) {
        printf("Wasn't able to read MSR on any CPUs, boo (try running with sudo?)! Exiting...");
        exit(1);
    }


    printf("Detected %d CPUs, monitoring votage...\n", maxcpu);
    while (true) {
        for (int cpu = 0; cpu < maxcpu; cpu++) {
            result r = read_voltage(cpu);
            if (r.error) {  // weird because we already read it before
                printf("\nFAILED!\n");
                exit(1);
            }
            printf("CPU %d: %4.2f ", cpu, r.value / 8192.);
        }
        fflush(stdout);
        usleep(100000u);
        printf("\r");
    }
    return 0;
}
