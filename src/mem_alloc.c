#include "myc/core.h"
#include "./_memory_.h"

/* Allocates a memory chunk of at least 'size' bytes. */
void* myc_mem_arena_malloc(MycMemArena_t *arena, uint32_t size)
{
    MYC_UNUSED(arena);
    MYC_UNUSED(size);
    MYC_NOT_IMPLEMENTED();

    return MYC_MEM_ALLOC_FAILED;
}

/* Resizes the memory chunk at 'addr' to be at least 'new_size' bytes, moves it if necessary and returns the new address. 
!!NOTE: Absolute pointers into the memory will be invalid if the chunk moves. */
void* myc_mem_arena_realloc(void *addr, uint32_t new_size)
{
    MYC_UNUSED(addr);
    MYC_UNUSED(new_size);
    MYC_NOT_IMPLEMENTED();

    return MYC_MEM_ALLOC_FAILED;
}

/* Frees the memory chunk at 'addr', allowing it to be reused. */
void myc_mem_arena_free(void *addr)
{
    MYC_UNUSED(addr);
    MYC_NOT_IMPLEMENTED();
}
