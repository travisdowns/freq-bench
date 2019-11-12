#include "misc.hpp"

#include <immintrin.h>
#include <x86intrin.h> // this is needed to have _mm_clflush

void clflush(const void *storage, size_t size) {
    for (char *p = (char *)storage, *e = p + size; p < e; p += 64) {
        _mm_clflush(p);
    }

    _mm_mfence();
}
