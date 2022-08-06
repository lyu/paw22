/* For license: see LICENSE file at top-level */

#ifndef _SHMEMA_MEMALLOC_H
#define _SHMEMA_MEMALLOC_H 1

#include <sys/types.h>          /* size_t */

/*
 * memory allocation
 */
void shmema_init(void *base, size_t capacity);
void shmema_finalize(void);
void *shmema_base(void);
void *shmema_malloc(size_t size);
void *shmema_calloc(size_t count, size_t size);
void shmema_free(void *addr);
void *shmema_realloc(void *addr, size_t new_size);
void *shmema_align(size_t alignment, size_t size);

void shmema_buddy_init(void *base, size_t capacity, int8_t id);
void shmema_buddy_finalize(int8_t id);
void *shmema_buddy_malloc(size_t size, int8_t id);
void shmema_buddy_free(void *addr, int8_t id);

#endif /* ! _SHMEMA_MEMALLOC_H */
