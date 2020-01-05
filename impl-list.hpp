#ifndef IMPL_LIST_H_
#define IMPL_LIST_H_

#include "common-cxx.hpp"

#include <vector>
#include <string>

/**
 * Offers access to all the intersection implementations.
 */

enum AlgoFlags {
    NONE         = 0,
    /** algo is too slow to run at default array sizes */
    SLOW         = 1 << 0,
    /** algo doesn't return the right result (e.g., because it is a dummy for testing) */
    INCORRECT    = 1 << 1,
    NO_VZ        = 1 << 2,
};

struct test_description {
    const char *name;
    memcpy_fn *f;
    const char *desc;
    AlgoFlags flags;

    void call_f(const bench_args& args) const {
        f(args);
    }
};

/**
 * Return the benchmark exactly matching the given name, or nullptr
 * if not found.
 */
const test_description* get_by_name(const std::string& name);

/**
 * Given a comma separated list of test names, return a list of all the
 * tests, or throw if one isn't found.
 */
std::vector<test_description> get_by_list(const std::string& list);

/**
 * Return all test descriptors.
 */
const std::vector<test_description>& get_all();

#endif
