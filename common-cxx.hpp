#ifndef SI_COMMON_CXX_H_
#define SI_COMMON_CXX_H_

/* to align with C++20 std::span */
#define span_CONFIG_INDEX_TYPE size_t

#include "hedley.h"
#include "inttypes.h"
#include "nonstd/span.hpp"

#include <stdlib.h>

using char_span = nonstd::span<char>;

/**
 * Bundles all the arguments.
 */
struct bench_args {
    void* dest;
    const void* source;
    size_t size;
};

using memcpy_fn = void (bench_args args);

#endif
