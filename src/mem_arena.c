#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "myc/core.h"
#include "./_memory_.h"



/* Libc mmap wrapper. */
static inline void* mem_mmap(size_t size) 
{
    void *mem = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    MYC_ASSERT(!(mem == MAP_FAILED && errno == EINVAL), "Invalid mmap parameters.");
    return mem;
}

/* Libc munmap wrapper. */
static inline int mem_munmap(void *mem, size_t size)
{
    int err = munmap(mem, size);
    MYC_ASSERT(!(err < 0 && errno == EINVAL), "Invalid munmap parameters.");
    MYC_ASSERT(!(err < 0 && errno == ENOMEM), "Partial unmapping.");
    return err;
}



// === CREATE / DESTROY ============================================================================================ //

static myc_err_t mem_arena_create_internal(MycMemArena_t **new_arena, uint32_t size);
static void mem_arena_reset_layout(MycMemArena_t *arena);

/* Creates a new memory arena with a capacity of at least 'size' bytes. */
myc_err_t myc_mem_arena_create(MycMemArena_t **new_arena, uint32_t size)
{
    myc_err_t exit_code;
    MycMemArena_t *arena;
    if ((exit_code = mem_arena_create_internal(&arena, size)) != MYC_SUCCESS) {
        return exit_code;
    }
    arena->head = arena;
    *new_arena = arena;
    return MYC_SUCCESS;
}

/* Expands the memory arena by creating a new arena of at least 'add_size' bytes, which is added as a child.
!!NOTE: The newly created memory region need not be contiguous to existing memory region(s). */
myc_err_t myc_mem_arena_expand(MycMemArena_t *arena, uint32_t add_size)
{
    myc_err_t exit_code;
    MycMemArena_t *add_arena;
    if ((exit_code = mem_arena_create_internal(&add_arena, add_size)) != MYC_SUCCESS) {
        return exit_code;
    }
    add_arena->head = arena;
    add_arena->next = arena->next;
    arena->next = add_arena;
    return MYC_SUCCESS;
}

/* Destroys the memory arena and releases the resources back to the OS. */
void myc_mem_arena_destroy(MycMemArena_t *arena)
{
    myc_err_t exit_code = MYC_SUCCESS;
    while (arena != NULL) {
        MycMemArena_t *next_arena = arena->next;
        if (mem_munmap(arena, arena->size) != 0) {
            MYC_LOG_TRACE("'munmap' failed at %p.   =>   %s.", arena, strerror(errno));
            exit_code = MYC_FAILED;
        }
        arena = next_arena;
    }

    if (exit_code != MYC_SUCCESS) {
        MYC_LOG_WARN("Could not fully destroy memory arena.");
    }
    /* Returning the error is ommited, because there is nothing we can do 
    and the OS will clean up on program termination anyway. */
}

/* Resets the memory arena by freeing all currently allocated memory chunks. This does not release resources to the OS. */
void myc_mem_arena_reset(MycMemArena_t *arena)
{
    for (MycMemArena_t *arena_i = arena; arena_i != NULL; arena_i = arena_i->next) {
        mem_arena_reset_layout(arena_i);
    }
}

static inline size_t calc_mem_arena_allocation_size(uint32_t requested_size)
{
    typedef uint32_t MycMemLayoutNode_t;
    const size_t SYSTEM_PAGE_SIZE = (size_t)sysconf(_SC_PAGE_SIZE);
    MYC_ASSERT(MYC_IS_POWER_OFF_TWO(SYSTEM_PAGE_SIZE), "Broken system page size.");

    const size_t user_size = MYC_QUANTIZE_UP((size_t)requested_size, MYC_MEM_ARENA_PAGE_SIZE);
    const size_t page_count = user_size / MYC_MEM_ARENA_PAGE_SIZE;
    const size_t max_bucket_node_count = page_count / 2 + 1;
    const size_t max_free_size_node_count = calc_free_size_node_count(max_bucket_node_count);
    const size_t max_layout_size = ((max_bucket_node_count + 1) * sizeof(MycMemLayoutNode_t)) + (max_free_size_node_count * sizeof(MycMemLayoutNode_t));
    const size_t total_size = sizeof(MycMemArena_t) + max_layout_size + user_size;
    const size_t allocation_size = MYC_QUANTIZE_UP(total_size, SYSTEM_PAGE_SIZE);
    return allocation_size;
}

static inline size_t calc_mem_arena_page_count(size_t allocation_size)
{
    /* The steps below try to kind of visualize the page count calculation, but to be honest the maths is kind of a mess.
    In essence it is the formula for calculating the allocation size above and solving for page count (with some simplification), which boils down to:
    #pages = [total_size - sizeof(Arena_t) - 4*sizeof(LayoutNode_t) - (sizeof(LayoutNode_t) / (#NODE_CHILD - 1))]  
           / [ARENA_PAGE_SIZE + sizeof(LayoutNode_t) + (sizeof(LayoutNode_t) / 2(#NODE_CHILD - 1))]. */

    typedef uint32_t MycMemLayoutNode_t;
    const size_t DIV_ERR_SCALAR = 2 * (MYC_MEM_LAYOUT_NODE_CHILD_COUNT - 1);    // Scalar used to avoid integer division rounding errors.
    const size_t static_memory_cost = ((sizeof(MycMemArena_t) + (4 * sizeof(MycMemLayoutNode_t))) * DIV_ERR_SCALAR) + (2 * sizeof(MycMemLayoutNode_t));
    const size_t memory_budget = (allocation_size * DIV_ERR_SCALAR) - static_memory_cost;
    const size_t memory_cost = ((MYC_MEM_ARENA_PAGE_SIZE + sizeof(MycMemLayoutNode_t)) * DIV_ERR_SCALAR) + sizeof(MycMemLayoutNode_t);
    const size_t page_count = memory_budget / memory_cost;
    return page_count;
}

static myc_err_t mem_arena_create_internal(MycMemArena_t **new_arena, uint32_t size)
{
    const size_t allocation_size = calc_mem_arena_allocation_size(size);
    const size_t page_count = calc_mem_arena_page_count(allocation_size);
    const size_t user_size = page_count * MYC_MEM_ARENA_PAGE_SIZE;
    const size_t max_bucket_count = page_count / 2 + 1;

    if (allocation_size > MYC_MEM_ARENA_SIZE_MAX) {
        MYC_LOG_TRACE("Allocation size (%lu) exceeds maximum arena size (%lu)", allocation_size, MYC_MEM_ARENA_SIZE_MAX);
        return MYC_ERR_INVALID_ARGUMENT;
    }

    MycMemArena_t *arena = mem_mmap(allocation_size);
    if (arena == MAP_FAILED) {
        MYC_LOG_TRACE("'mmap' failed.   =>   %s.", strerror(errno));
        return MYC_ERR_NO_MEMORY;
    }

    arena->size = allocation_size;
    arena->internal_size = allocation_size - user_size;
    arena->head = NULL;
    arena->next = NULL;
    arena->layout.bucket_offsets = (void*)arena + sizeof(MycMemArena_t);
    arena->layout.max_free_sizes = arena->layout.bucket_offsets + max_bucket_count + 1;     // Add extra bucket as end marker.
    mem_arena_reset_layout(arena);
    *new_arena = arena;
    return MYC_SUCCESS;
}

static void mem_arena_reset_layout(MycMemArena_t *arena)
{
    arena->layout.bucket_node_count = 1;
    arena->layout.parent_node_count = calc_parent_node_count(1);
    arena->layout.bucket_offsets[0] = 0;
    arena->layout.bucket_offsets[1] = arena->size;
    arena->layout.max_free_sizes[0] = arena->size - arena->internal_size;
}



// === INTROSPECTION =============================================================================================== //

static inline void mem_arena_print_global_info(const MycMemArena_t *arena);
static inline void mem_arena_print_local_info(const MycMemArena_t *arena);
static inline void mem_arena_print_chunks_info(const MycMemArena_t *arena);

/* Prints memory usage/layout information to stdout. */
void myc_mem_arena_introspect(const MycMemArena_t *arena)
{
    printf("["MYC_FMT_GREEN("MEMORY INTROSPECT")"]:\n");
    mem_arena_print_global_info(arena);

    printf("  |   Regions:\n");
    for (const MycMemArena_t *arena_i = arena; arena_i != NULL; arena_i = arena_i->next) {
        mem_arena_print_local_info(arena_i);
        mem_arena_print_chunks_info(arena_i);
    }
}

static inline void mem_arena_print_global_info(const MycMemArena_t *arena)
{
    size_t region_count = 0;
    size_t total_size = 0;
    size_t user_size = 0;
    for (const MycMemArena_t *arena_i = arena; arena_i != NULL; arena_i = arena_i->next) {
        region_count += 1;
        total_size += arena_i->size;
        user_size += arena_i->size - arena_i->internal_size;
    }

    printf("  |   Memory Arena:   < region count: "MYC_FMT_BOLD("%lu"), region_count);
    printf(" | user size: "MYC_FMT_BOLD("%.2f KiB"), (float)user_size / 1024.0f);
    printf(" total size: "MYC_FMT_BOLD("%.2f KiB")" >\n", (float)total_size / 1024.0f);
}

static inline void mem_arena_print_local_info(const MycMemArena_t *arena)
{
    const size_t user_size = arena->size - arena->internal_size;
    size_t size_used = 0;
    for (size_t bucket_idx = 0; bucket_idx < arena->layout.bucket_node_count; ++bucket_idx) {
        size_used += mem_layout_bucket_size_used(&arena->layout, bucket_idx);
    }
    size_used -= arena->internal_size;

    printf("  |       - Region at "MYC_FMT_BOLD("0x%012lx")":", (size_t)arena);
    printf("   < capacity: "MYC_FMT_BOLD("%.2f KiB"), (float)user_size / 1024.0f);
    printf(" | size used: "MYC_FMT_BOLD("%.2f KiB")" ("MYC_FMT_BOLD("%.1f%%")") >\n", 
            (float)size_used / 1024.0f, 100.0f * (float)size_used / (float)user_size);
}

static inline void mem_arena_print_chunks_info(const MycMemArena_t *arena)
{
    char buffer[128];
    printf("  |            - Chunk at "MYC_FMT_BOLD("0x00000000")":");
    snprintf(buffer, sizeof(buffer), "     < state: INTERNAL | size: "MYC_FMT_BOLD("%lu bytes")" >", arena->internal_size);
    printf("%-56s", buffer);

    uint32_t chunk_offset = (uint32_t)arena->internal_size;
    for (size_t bucket_idx = 0; bucket_idx < arena->layout.bucket_node_count; ++bucket_idx) {
        while (chunk_offset < mem_layout_bucket_free_offset(&arena->layout, bucket_idx)) {    
            MycMemChunk_t *chunk = mem_chunk_at(arena, chunk_offset);
            MYC_ASSERT(chunk->size > 0, "Chunk size is never 0");
            printf("\n  |            - Chunk at "MYC_FMT_BOLD("0x%08x")":", chunk->offset);
            snprintf(buffer, sizeof(buffer), "     < state: ALLOCATED | size: "MYC_FMT_BOLD("%u bytes")" >", chunk->size);
            printf("%-56s", buffer);
            chunk_offset += chunk->size;
        }
        uint32_t free_size = mem_layout_bucket_free_size(&arena->layout, bucket_idx);
        if (free_size > 0) {
            printf("\n  |            - Chunk at "MYC_FMT_BOLD("0x%08x")":", chunk_offset);
            snprintf(buffer, sizeof(buffer), "     < state: FREE | size: "MYC_FMT_BOLD("%u bytes")" >", free_size);
            printf("%-56s", buffer);
        }
        printf("   (BUCKET END)");
        chunk_offset += free_size;
    }
    printf("\n  |\n");
}