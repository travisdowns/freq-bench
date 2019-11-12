/*
 * huge-alloc.cpp
 */

#include "huge-alloc.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>

#include <sys/mman.h>
#include <linux/kernel-page-flags.h>

#include "page-info.h"

#define HUGE_PAGE_SIZE ((size_t)(2u * 1024u * 1024u))
#define HUGE_PAGE_MASK ((size_t)-HUGE_PAGE_SIZE)

/* allocate size bytes of storage in a hugepage */
void *huge_alloc(size_t user_size, bool print) {
    if (user_size > MAX_HUGE_ALLOC) {
        fprintf(stderr, "request exceeds MAX_HUGE_ALLOC in %s, check your math\n", __func__);
        return 0;
    }

    // we request size + 2 * HUGE_PAGE_SIZE so we'll always have at least one huge page boundary in the allocation
    size_t mmap_size = user_size + 2 * HUGE_PAGE_SIZE;

    char *mmap_p = (char *)mmap(0, mmap_size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

    if (mmap_p == MAP_FAILED) {
        fprintf(stderr, "MMAP failed in %s\n", __func__);
        return 0;
    }

    // align up to a hugepage boundary
    char *aligned_p = (char *)(((uintptr_t)mmap_p + HUGE_PAGE_SIZE) & HUGE_PAGE_MASK);

    madvise(aligned_p, user_size + HUGE_PAGE_SIZE, MADV_HUGEPAGE);

    // touch the memory so we can get stats on it
    memset(aligned_p, 0xCC, user_size);

    if (print) {
        page_info_array info = get_info_for_range(aligned_p, aligned_p + user_size);
        flag_count fcount = get_flag_count(info, KPF_THP);
        if (user_size > 0 && fcount.pages_available == 0) {
            fprintf(stderr, "failed to get any huge page info - probably you need to run as root\n");
        } else {
            fprintf(stderr, "hugepage ratio %4.3f (available %4.3f) for allocation of size %zu\n",
                (double)fcount.pages_set/fcount.pages_available,
                (double)fcount.pages_available/fcount.pages_total,
                user_size);
        }
        free_info_array(info);
    }

    return aligned_p;
}
