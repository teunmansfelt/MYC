#include "myc/core.h"
#include "./_memory_.h"

/* Creates a new bump allocator with a capacity of at least 'size' bytes. */
myc_err_t myc_mem_bump_alloc_create(MycMemBumpAlloc_t **new_bump_alloc, MycMemArena_t *arena, uint32_t size)
{
    MycMemBumpAlloc_t *bump_alloc = myc_mem_arena_malloc(arena, size + sizeof(MycMemBumpAlloc_t));
    if (bump_alloc == MYC_MEM_ALLOC_FAILED) {
        MYC_LOG_TRACE("Cannot allocate enough memory.");
        return MYC_ERR_NO_MEMORY;
    }

    uint32_t chunk_size = myc_mem_arena_get_chunk_size(bump_alloc);
    bump_alloc->node.capacity = chunk_size;
    bump_alloc->node.size_used = sizeof(MycMemBumpAlloc_t);
    bump_alloc->node.next = NULL;
    bump_alloc->arena = arena;
    bump_alloc->current = &bump_alloc->node;
    bump_alloc->last = &bump_alloc->node;
    *new_bump_alloc = bump_alloc;
    return MYC_SUCCESS;
}

/* Expands the bump allocator by creating a new allocator of at least 'add_size' bytes and adding it as a child. */
myc_err_t myc_mem_bump_alloc_expand(MycMemBumpAlloc_t *bump_alloc, uint32_t add_size)
{
    MycMemBumpAllocNode_t *node = myc_mem_arena_malloc(bump_alloc->arena, add_size + sizeof(MycMemBumpAllocNode_t));
    if (node == MYC_MEM_ALLOC_FAILED) {
        MYC_LOG_TRACE("Cannot allocate enough memory.");
        return MYC_ERR_NO_MEMORY;
    }

    uint32_t chunk_size = myc_mem_arena_get_chunk_size(node);
    node->capacity = chunk_size;
    node->size_used = sizeof(MycMemBumpAllocNode_t);
    node->next = NULL;
    bump_alloc->last->next = node;
    bump_alloc->last = node;
    return MYC_SUCCESS;
}

/* Destroys the bump allocator and frees all memory allocated by it. */
void myc_mem_bump_alloc_destroy(MycMemBumpAlloc_t *bump_alloc)
{
    myc_mem_arena_free(bump_alloc);
    for (MycMemBumpAllocNode_t *node = bump_alloc->node.next; node != NULL; node = node->next) {
        myc_mem_arena_free(node);
    }
}

/* Allocates 'size' bytes on the bump allocator, aligned to a multiple of 'alignment' 
!!NOTE: The given alignment must be a power of two. */
void* myc_mem_bump_aligned_malloc(MycMemBumpAlloc_t *bump_alloc, uint32_t size, size_t alignment)
{
    MYC_ASSERT(MYC_IS_POWER_OFF_TWO(alignment), "Given alignment must be a power of two.");

    for (MycMemBumpAllocNode_t *node = bump_alloc->current; node != NULL; node = node->next) {
        void *const end_ptr = mem_bump_alloc_node_end_ptr(node);
        void *const free_ptr = mem_bump_alloc_node_free_ptr(node);
        void *const aligned_free_ptr = (void*)MYC_QUANTIZE_UP((size_t)free_ptr, alignment);
        if (aligned_free_ptr + size <= end_ptr) {
            node->size_used += size + (aligned_free_ptr - free_ptr);
            bump_alloc->current = node;
            return aligned_free_ptr;
        }
    }
    return MYC_MEM_ALLOC_FAILED;
}

/* Returns the number of contiguous bytes still available. */
uint32_t myc_mem_bump_alloc_get_free_size(MycMemBumpAlloc_t *bump_alloc)
{
    return bump_alloc->current->capacity - bump_alloc->current->size_used;
}

/* Resets the bump allocator as if no allocations were made previously. */
void myc_mem_bump_alloc_reset(MycMemBumpAlloc_t *bump_alloc)
{
    bump_alloc->node.size_used = sizeof(MycMemBumpAlloc_t);
    for (MycMemBumpAllocNode_t *node = bump_alloc->node.next; node != NULL; node = node->next) {
        node->size_used = sizeof(MycMemBumpAllocNode_t);
    }
}
