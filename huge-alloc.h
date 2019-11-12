/*
 * huge-alloc.hpp
 *
 * Inefficient allocator that allows allocating memory regions backed by THP pages.
 */

#ifndef HUGE_ALLOC_HPP_
#define HUGE_ALLOC_HPP_

#include <stddef.h>
#include <stdbool.h>

// 1152921504606846976 bytes should be enough for everyone
#define MAX_HUGE_ALLOC (1ULL << 60)

#ifdef __cplusplus
extern "C" {
#endif

/* allocate size bytes of storage in a hugepage */
void *huge_alloc(size_t size, bool print);

/* free the pointer pointed to by p */
void huge_free(void *p);

#ifdef __cplusplus
}
#endif


#endif /* HUGE_ALLOC_HPP_ */
