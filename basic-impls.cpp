/**
 * Implementation of various very basic algorithms, mostly as a litmus test for the more complicated ones.
 */

#include "basic-impls.hpp"
#include "algo-common.hpp"

#include <assert.h>
#include <inttypes.h>
#include <string.h>
#include <numeric>

void memcpy_stdlib(bench_args args) {
    memcpy(args.dest, args.source, args.size);
}

void memmove_stdlib(bench_args args) {
    memmove(args.dest, args.source, args.size);
}

void repmovsb(bench_args args) {
    asm volatile ("rep movsb\n"
        : "+S"(args.source), "+D"(args.dest), "+c"(args.size)
        :
        : "memory"
    );
}

void vporxmm(bench_args args) {
    asm volatile ("vpor %xmm0, %xmm0, %xmm0\n");
}

void vporymm(bench_args args) {
    asm volatile ("vpor %ymm0, %ymm0, %ymm0\n");
}

void vporzmm(bench_args args) {
    asm volatile ("vpord %zmm0, %zmm0, %zmm0\n");
}

void vporxmm_vz(bench_args args) {
    asm volatile (
        "vpor %xmm0, %xmm0, %xmm0\n"
        "vzeroupper\n"
    );
}

void vporymm_vz(bench_args args) {
    asm volatile (
        "vpor %ymm0, %ymm0, %ymm0\n"
        "vzeroupper\n"
    );
}

void vporzmm_vz(bench_args args) {
    asm volatile (
        "vpord %zmm0, %zmm0, %zmm0\n"
        "vzeroupper\n"
    );
}

/**
 * 1000 copies of instr
 */
#define MAKE_MANY(name,instr,regd,regs) \
void name##_vz100(bench_args args) {  \
    asm volatile (                     \
        ".rept 1000\n\t"               \
        #instr " %" #regs ", %" #regs ", %" #regd "\n\t" \
        ".endr\n\t"                    \
        "vzeroupper\n\t"               \
    );                                 \
}

// latency
MAKE_MANY(vporxmm, vpor,  xmm0, xmm0)
MAKE_MANY(vporymm, vpor,  ymm0, ymm0)
MAKE_MANY(vporzmm, vpord, zmm0, zmm0)

// throughput
MAKE_MANY(vporxmm_tput, vpor,  xmm0, xmm1)
MAKE_MANY(vporymm_tput, vpor,  ymm0, ymm1)
MAKE_MANY(vporzmm_tput, vpord, zmm0, zmm1)

// vpermd
MAKE_MANY(vpermdzmm     , vpermd, zmm0, zmm0)
MAKE_MANY(vpermdzmm_tput, vpermd, zmm0, zmm1)



void dummy(bench_args args) {}
