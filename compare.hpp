#ifndef COMPARE_H_
#define COMPARE_H_

/*
 * Comparison routines for primitive values, suitable for passing to qsort and
 * similar methods expecting comparator functions that accept pointers to their
 * arguments and return <0, 0 or >0 depending on the three-way result.
 */

#include <inttypes.h>

#define COMPARE_FN(suffix, T)                                \
    int compare_ ## suffix(const void *l_, const void *r_) { \
        T l = *(const T *)l_;                                \
        T r = *(const T *)r_;                                \
        return (l > r) - (l < r);                            \
    }                                                        \

COMPARE_FN(u32, uint32_t)
COMPARE_FN(u64, uint64_t)
COMPARE_FN(i32,  int32_t)
COMPARE_FN(i64,  int64_t)

#ifdef __cplusplus
#include <type_traits>
template<typename T>
int compare_generic(const void *l_, const void *r_) {
    static_assert(std::is_integral<T>::value, "type is not integral");
    T l = *(const T *)l_;                            
    T r = *(const T *)r_;                            
    return (l > r) - (l < r);                        
}                                                    
#endif

#endif // COMPARE_H_
