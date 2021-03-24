#ifndef SI_COMMON_CXX_H_
#define SI_COMMON_CXX_H_

/* to align with C++20 std::span */
#define span_CONFIG_INDEX_TYPE size_t

#include <stdexcept> // needed by span.hpp

#include "hedley.h"
#include "inttypes.h"
#include "nonstd/span.hpp"

#include <stdlib.h>

using char_span = nonstd::span<char>;

/**
 * Bundles all the arguments.
 */
struct bench_args {};

using bench_fn = void (bench_args args);

#define ALL_RATIOS_X(f) \
    f(1) \
    f(2) \
    f(3) \
    f(4) \
    f(5) \
    f(6) \
    f(7) \
    f(8) \
    f(9) \
    f(10) \
    f(20) \
    f(30) \
    f(40) \
    f(50) \
    f(60) \
    f(70) \
    f(80) \
    f(90) \
    f(100) \
    f(120) \
    f(140) \
    f(160) \
    f(180) \
    f(200) \

#endif
