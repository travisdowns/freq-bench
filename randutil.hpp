#include "hedley.h"

#include <cstddef>
#include <vector>
#include <cassert>
#include <unordered_set>
#include <random>
#include <algorithm>
#include <type_traits>
#include <functional>

#include "pcg-cpp/pcg_random.hpp"
#include "simple-timer.hpp"

namespace detail {

/**
 * Here E only needs to be a callable, doens't need to satisfy the
 * engine concept, so it can be a distribution, etc.
 */
template <typename T, typename E>
std::vector<T> unique_impl(E& e, std::size_t n) {
    std::vector<T> ret(n);
    std::unordered_set<T> existing;
    for (size_t i = 0; i < n;) {
        auto val = e();
        if (existing.find(val) == existing.end()) {
            ret[i++] = val;
            existing.insert(val);
        }
    }
    return ret;
}

}

/**
 * Generate n numbers without repitition with the given engine e of type E.
 */
template <typename E>
HEDLEY_NEVER_INLINE
std::vector<typename E::result_type> random_unique(E& e, std::size_t n) {
    using T = typename E::result_type;
    assert(n <= E::max() / 2 && "n too big, implement the reverse operation");
    return detail::unique_impl<T>(e, n);
}

/**
 * Generate n numbers without repitition with the given engine e of type E,
 * and the given distribution of type D.
 */
template <typename E, typename D>
HEDLEY_NEVER_INLINE
std::vector<typename E::result_type> random_unique(E& e,  D& d, std::size_t n) {
    using T = typename D::result_type;
    assert(n <= d.max() / 2 && "n too big, implement the reverse operation");
    auto l = [&e, &d]() -> T { return d(e); };
    return detail::unique_impl<T>(l, n);
}

/**
 * Sample n unique elements (i.e., elements at unique positions, if the input
 * container has duplicates the output may as well) from collection c.
 */
template <typename E, typename C>
HEDLEY_NEVER_INLINE
std::vector<typename E::result_type> sample_unique(E& e, const C& c, std::size_t n) {
    using T = typename E::result_type;
    assert(c.size() <= E::max() / 2); // it's not going to work otherwise
    assert(n <= c.size() / 2); // otherwise implement another algorithm

    std::uniform_int_distribution<size_t> dist(0, c.size() - 1);

    auto rng = [&](){ return dist(e); };
    auto indices = detail::unique_impl<T>(rng, n);

    std::vector<T> ret;
    for (auto i : indices) {
        ret.push_back(c[i]);
    }
    return ret;
}

/**
 * Override output array with count random elements from
 * the given generator.
 */
template <typename E, typename T>
void randomize(E& e, T *output, size_t count) {
    std::generate(output, output + count, std::ref(e));
}

template <typename T, typename E>
std::vector<T> random_vector(E& e, size_t count) {
    std::vector<T> ret(count);
    randomize(e, ret.data(), count);
    return ret;
}

template <size_t S>
struct pick_pcg_base {
    static_assert(S < S, "no appropriate pcg for types of this size"); // S < S delays static asset eval until instantiation
};

template<>
struct pick_pcg_base<32> {
    using type = pcg32;
};

template<>
struct pick_pcg_base<64> {
    using type = pcg64;
};

/**
 * Given a type, picks a pcg generator of the appropriate size,
 * or else fails. It picks pcg32 for everything 4 byte or less
 * and pcg64 for 8 type things.
 */
template <typename T>
struct pick_pcg : public pick_pcg_base<sizeof(T) == 8 ? 64 : (sizeof(T) <= 4 ? 32 : 0)> {
    static_assert(std::is_integral<T>::value, "this makes sense for integral types only");
};


