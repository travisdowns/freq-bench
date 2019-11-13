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
