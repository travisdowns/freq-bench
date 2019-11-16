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

void dummy(bench_args args) {}
