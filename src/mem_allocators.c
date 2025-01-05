#include "myc/core.h"
#include "./_memory_.h"

/* Creates a new bump allocator with a capacity of at least 'size' bytes. */
myc_err_t myc_mem_bump_alloc_create(MycMemBumpAlloc_t **new_bump_alloc, MycMemArena_t *arena, uint32_t size)
{
    MycMemBumpAlloc_t *bump_alloc = myc_mem_arena_malloc(arena, size + sizeof(MycMemBumpAlloc_t));
    if (bump_alloc == MYC_MEM_ALLOC_FAILED) {
        MYC_LOG_TRACE("Not enough memory.");
        return MYC_ERR_NO_MEMORY;
    } 
    MycMemChunk_t *chunk = mem_chunk_from_addr(bump_alloc);
    bump_alloc->capacity = chunk->size - sizeof(MycMemChunk_t);
    bump_alloc->size_used = sizeof(MycMemBumpAlloc_t);
    bump_alloc->next = NULL;
    *new_bump_alloc = bump_alloc;
    return MYC_SUCCESS;
}

/* Expands the bump allocator by creating a new allocator of at least 'add_size' bytes and adding it as a child. */
myc_err_t myc_mem_bump_alloc_expand(MycMemBumpAlloc_t *bump_alloc, uint32_t add_size)
{
    myc_err_t exit_code;
    MycMemArena_t *arena = mem_chunk_get_arena(mem_chunk_from_addr(bump_alloc));
    MycMemBumpAlloc_t *add_bump_alloc;
    if ((exit_code = myc_mem_bump_alloc_create(&add_bump_alloc, arena, add_size)) != MYC_SUCCESS) {
        return exit_code;
    }
    add_bump_alloc->next = bump_alloc->next;
    bump_alloc->next = add_bump_alloc;
    return MYC_SUCCESS;
}

/* Destroys the bump allocator and frees all memory allocated by it. */
void myc_mem_bump_alloc_destroy(MycMemBumpAlloc_t *bump_alloc)
{
    for (MycMemBumpAlloc_t *bump_alloc_i = bump_alloc; bump_alloc_i != NULL; bump_alloc_i = bump_alloc_i->next) {
        myc_mem_arena_free(bump_alloc_i);
    }
}

/* Resets the bump allocator as if no allocations were made previously. */
void myc_mem_bump_alloc_reset(MycMemBumpAlloc_t *bump_alloc)
{
    for (MycMemBumpAlloc_t *bump_alloc_i = bump_alloc; bump_alloc_i != NULL; bump_alloc_i = bump_alloc_i->next) {
        bump_alloc_i->size_used = sizeof(MycMemBumpAlloc_t);
    }
}

/* Allocates 'size' bytes on the bump allocator, aligned to a multiple of 'alignment' 
!!NOTE: The given alignment must be a power of two. */
void* myc_mem_bump_aligned_malloc(MycMemBumpAlloc_t *bump_alloc, uint32_t size, uint32_t alignment)
{
    MYC_ASSERT(MYC_IS_POWER_OFF_TWO(alignment), "Alignment must be a power of two.");

    myc_ptr_value_t end_ptr = (myc_ptr_value_t)bump_alloc + bump_alloc->capacity;
    myc_ptr_value_t free_ptr = (myc_ptr_value_t)bump_alloc + bump_alloc->size_used;
    myc_ptr_value_t aligned_free_ptr = MYC_QUANTIZE_UP(free_ptr, alignment);
    if (aligned_free_ptr + size > end_ptr) {
        return MYC_MEM_ALLOC_FAILED;
    }
    bump_alloc->size_used += size + (aligned_free_ptr - free_ptr);
    void *addr = (void*)aligned_free_ptr;
    return addr;
}
