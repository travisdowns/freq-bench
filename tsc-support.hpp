#ifndef TSC_SUPPORT_H_
#define TSC_SUPPORT_H_

#include <inttypes.h>
#include <stdbool.h>

#ifdef _MSC_VER
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif


static inline uint64_t rdtsc() {
    return __rdtsc();
}

static inline uint64_t rdtscp() {
   uint64_t hi, lo;
   __asm__ ("rdtscp" : "=a"(lo), "=d"(hi) : : "rcx");
   return (hi << 32) | lo;

    // the ideal implementation is below, but gcc likes to acually do the dead write to cpu,
    // so we use inline assembly instead
    // unsigned int cpu;
    // return __rdtscp(&cpu);
}

/**
 * Get the TSC frequency.
 *
 * By default, this tries to read the TSC frequency directly from cpuid leaf 0x15,
 * if it is on a supported architecture, otherwise it falls back to using a calibration
 * loop. If force_calibrate is true, it always uses the calibration loop and never reads
 * from cpuid.
 */
uint64_t get_tsc_freq(bool force_calibrate);

/** return a string describing how the TSC frequency was determined */
const char* get_tsc_cal_info(bool force_calibrate);

#ifdef __cplusplus
}
#endif

#endif
