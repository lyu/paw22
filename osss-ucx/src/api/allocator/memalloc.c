/* For license: see LICENSE file at top-level */

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h> /* TODO: remove this */

#include "internal-malloc.h"

#include "memalloc.h"

/**
 * the memory area we manage in this unit.
 *
 * Not visible to anyone else
 */
static mspace myspace;

/**
 * initialize the memory pool
 */
void
shmema_init(void *base, size_t capacity)
{
    myspace = create_mspace_with_base(base, capacity, 1);
}

/**
 * clean up memory pool
 */
void
shmema_finalize(void)
{
    destroy_mspace(myspace);
}

/**
 * return start of pool
 */
void *
shmema_base(void)
{
    return myspace;
}

/**
 * allocate SIZE bytes from the pool
 */

void *
shmema_malloc(size_t size)
{
    return mspace_malloc(myspace, size);
}

/**
 * allocate COUNT * SIZE bytes, zero out
 */

void *
shmema_calloc(size_t count, size_t size)
{
    return mspace_calloc(myspace, count, size);
}

/**
 * release memory previously allocated at ADDR
 */
void
shmema_free(void *addr)
{
    mspace_free(myspace, addr);
}

/**
 * resize ADDR to NEW_SIZE bytes
 */
void *
shmema_realloc(void *addr, size_t new_size)
{
    return mspace_realloc(myspace, addr, new_size);
}

/**
 * allocate memory of SIZE bytes, aligning to ALIGNMENT
 */
void *
shmema_align(size_t alignment, size_t size)
{
    return mspace_memalign(myspace, alignment, size);
}

/* =============== Buddy allocator =============== */
/* 4K allocation granularity */
#define BUDDY_GRAIN_LOG 12

#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#define IS_POW_OF_2(n) (((n) & ((n) - 1)) == 0)


typedef struct buddy {
    uint8_t height;
    int8_t tree[0];
} buddy_t;


uint32_t get_parent_idx(const uint32_t idx)
{
    return (idx + 1) / 2 - 1;
}


uint32_t pow2(const int8_t e)
{
    if (e >= 0) {
        return 1U << e;
    } else {
        return 0;
    }
}


uint32_t get_idx_offset(const uint32_t idx,
                        const uint8_t level_exp,
                        const uint8_t height)
{
    return ((idx + 1) << level_exp) - (1U << height);
}


buddy_t* buddy_new(const uint8_t height)
{
    const uint32_t tree_sz = (1U << (height + 1)) - 1;
    buddy_t* bp            = malloc(sizeof(buddy_t) + tree_sz);
    bp->height             = height;

    uint8_t level_exp = height + 1;
    for (uint32_t i = 0; i < (1U << (height + 1)) - 1; i++) {
        if (IS_POW_OF_2(i + 1)) {
            level_exp--;
        }
        bp->tree[i] = level_exp;
    }

    return bp;
}


void buddy_del(buddy_t* bp)
{
    free(bp);
}


uint32_t buddy_alloc(buddy_t* bp, uint32_t n_units)
{
    if ((pow2(bp->tree[0]) < n_units) || (n_units == 0)) {
        return -1;
    } else if (!IS_POW_OF_2(n_units)) {
        n_units |= n_units >> 1;
        n_units |= n_units >> 2;
        n_units |= n_units >> 4;
        n_units |= n_units >> 8;
        n_units |= n_units >> 16;
        n_units++;
    }

    uint32_t idx      = 0;
    uint8_t level_exp = bp->height;

    while ((1U << level_exp) > n_units) {
        if (pow2(bp->tree[idx * 2 + 1]) >= n_units) {
            idx = idx * 2 + 1;
        } else {
            idx = idx * 2 + 2;
        }
        level_exp--;
    }

    bp->tree[idx]         = -1;
    const uint32_t offset = get_idx_offset(idx, level_exp, bp->height);

    while (idx != 0) {
        idx           = get_parent_idx(idx);
        bp->tree[idx] = MAX(bp->tree[idx * 2 + 1], bp->tree[idx * 2 + 2]);
    }

    return offset;
}


void buddy_free(buddy_t* bp, const uint32_t offset)
{
    assert(offset < (1U << bp->height));

    uint32_t idx = (1U << bp->height) - 1 + offset;

    uint8_t level_exp = 0;
    while (bp->tree[idx] != -1) {
        assert(idx != 0);
        idx = get_parent_idx(idx);
        level_exp++;
    }

    bp->tree[idx] = level_exp;
    while (idx != 0) {
        idx = get_parent_idx(idx);
        level_exp++;

        const int8_t max_l = bp->tree[idx * 2 + 1];
        const int8_t max_r = bp->tree[idx * 2 + 2];

        if ((max_l == max_r) && (max_l == level_exp - 1)) {
            bp->tree[idx] = level_exp;
        } else {
            bp->tree[idx] = MAX(max_l, max_r);
        }
    }
}

static buddy_t* bas[8];
static char* bases[8];

void
shmema_buddy_init(void *base, size_t capacity, int8_t id)
{
    const uint8_t height = log2(capacity >> BUDDY_GRAIN_LOG);
    bas[id] = buddy_new(height);
    bases[id] = base;
}

void
shmema_buddy_finalize(int8_t id)
{
    buddy_del(bas[id]);
}

void *
shmema_buddy_malloc(size_t size, int8_t id)
{
    uint32_t units = size >> BUDDY_GRAIN_LOG;

    if ((units << BUDDY_GRAIN_LOG) < size) {
        units++;
    }

    const uint32_t offset = buddy_alloc(bas[id], units);

    return bases[id] + (offset << BUDDY_GRAIN_LOG);
}

void
shmema_buddy_free(void *addr, int8_t id)
{
    const uint32_t offset = ((char*)addr - bases[id]) >> BUDDY_GRAIN_LOG;
    buddy_free(bas[id], offset);
}
