#ifndef _MYC_MEMORY_H_
#define _MYC_MEMORY_H_

#include "myc/types.h"

#define MYC_MEM_ALLOC_FAILED (void*)0

/* Opaque handle representing a memory arena. */
typedef struct _MycMemoryArena MycMemArena_t;

/* Creates a new memory arena with a capacity of at least 'size' bytes. */
myc_err_t myc_mem_arena_create(MycMemArena_t **new_arena, uint32_t size);
/* Expands the memory arena by creating a new arena of at least 'add_size' bytes, which is added as a child.
!!NOTE: The newly created memory region need not be contiguous to existing memory region(s). */
myc_err_t myc_mem_arena_expand(MycMemArena_t *arena, uint32_t add_size);
/* Destroys the memory arena and releases the resources back to the OS. */
void myc_mem_arena_destroy(MycMemArena_t *arena);

/* Allocates a memory chunk of at least 'size' bytes. */
void* myc_mem_arena_malloc(MycMemArena_t *arena, uint32_t size);
/* Resizes the memory chunk at 'addr' to be at least 'new_size' bytes, moves it if necessary and returns the new address. 
!!NOTE: Absolute pointers into the memory will be invalid if the chunk moves. */
void* myc_mem_arena_realloc(void *addr, uint32_t new_size);
/* Frees the memory chunk at 'addr', allowing it to be reused. */
void myc_mem_arena_free(void *addr);
/* Resets the memory arena by freeing all currently allocated memory chunks. This does not release resources to the OS. */
void myc_mem_arena_reset(MycMemArena_t *arena);

/* Prints memory usage/layout information to stdout. */
void myc_mem_arena_introspect(const MycMemArena_t *arena);


#endif // _MYC_MEMORY_H_