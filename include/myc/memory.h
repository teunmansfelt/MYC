#ifndef _MYC_MEMORY_H_
#define _MYC_MEMORY_H_

#include "myc/types.h"

#define MYC_MEM_ALLOC_FAILED (void*)0



// === MEMORY ARENA ================================================================================================ //

/* Opaque handle representing a memory arena. */
typedef struct _MycMemoryArena MycMemArena_t;

/* Creates a new memory arena with a capacity of at least 'size' bytes. */
myc_err_t myc_mem_arena_create(MycMemArena_t **new_arena, uint32_t size);
/* Expands the memory arena by creating a new arena of at least 'add_size' bytes and adding it as a child.
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

/* Returns the actual user size of the memory chunk at 'addr'. */
uint32_t myc_mem_arena_chunk_size(void *addr);
/* Prints memory usage/layout information to stdout. */
void myc_mem_arena_introspect(const MycMemArena_t *arena);



// === MEMORY ALLOCATORS =========================================================================================== //

/* Opaque handle representing a simple bump allocator. */
typedef struct _MycMemBumpAllocator MycMemBumpAlloc_t;

/* Creates a new bump allocator with a capacity of at least 'size' bytes. */
myc_err_t myc_mem_bump_alloc_create(MycMemBumpAlloc_t **new_bump_alloc, MycMemArena_t *arena, uint32_t size);
/* Expands the bump allocator by creating a new allocator of at least 'add_size' bytes and adding it as a child. */
myc_err_t myc_mem_bump_alloc_expand(MycMemBumpAlloc_t *bump_alloc, uint32_t add_size);
/* Destroys the bump allocator and frees all memory allocated by it. */
void myc_mem_bump_alloc_destroy(MycMemBumpAlloc_t *bump_alloc);

/* Allocates 'size' bytes on the bump allocator, aligned to a multiple of 'alignment' 
!!NOTE: The given alignment must be a power of two. */
void* myc_mem_bump_aligned_malloc(MycMemBumpAlloc_t *bump_alloc, uint32_t size, uint32_t alignment);
/* Allocates 'size' bytes on the bump allocator, aligned to a multiple of sizeof(void*). */
static inline void* myc_mem_bump_malloc(MycMemBumpAlloc_t *bump_alloc, uint32_t size) {
    return myc_mem_bump_aligned_malloc(bump_alloc, size, sizeof(void*));
}
/* Returns the number of contiguous bytes still available. */
uint32_t myc_mem_bump_alloc_free_size(MycMemBumpAlloc_t *bump_alloc);
/* Resets the bump allocator as if no allocations were made previously. */
void myc_mem_bump_alloc_reset(MycMemBumpAlloc_t *bump_alloc);



/* Opaque handle representing a frame allocator (aka tempory allocator). */
typedef struct _MycMemFrameAllocator MycMemFrameAlloc_t;

/* Creates a new frame allocator with a capacity of at least 'size' bytes. */
myc_err_t myc_mem_frame_alloc_create(MycMemFrameAlloc_t **new_frame_alloc, MycMemArena_t *arena, uint32_t size);
/* Expands the frame allocator by creating a new allocator of at least 'add_size' bytes and adding it as a child. */
myc_err_t myc_mem_frame_alloc_expand(MycMemFrameAlloc_t **new_frame_alloc, MycMemArena_t *arena, uint32_t size);

#endif // _MYC_MEMORY_H_