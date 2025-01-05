#include <string.h>

#include "myc/core.h"
#include "./_memory_.h"



// === ALLOC / REALLOC / FREE ====================================================================================== //

static myc_err_t mem_chunk_alloc(MycMemChunk_t **new_chunk, MycMemArena_t *arena, uint32_t size);
static MycMemChunkSearchInfo_t mem_chunk_find(const MycMemChunk_t *chunk);
static myc_err_t mem_chunk_resize(MycMemChunk_t *chunk, uint32_t new_size, MycMemChunkSearchInfo_t *chunk_info);
static void mem_chunk_free(MycMemChunk_t *chunk, MycMemChunkSearchInfo_t *chunk_info);
static void mem_chunk_revert(const MycMemChunk_t *chunk, MycMemChunkSearchInfo_t *chunk_info);

/* Allocates a memory chunk of at least 'size' bytes. */
void* myc_mem_arena_malloc(MycMemArena_t *arena, uint32_t size)
{
    if (size == 0) return MYC_MEM_ALLOC_FAILED;
    size = MYC_QUANTIZE_UP(size + sizeof(MycMemChunk_t), MYC_MEM_ARENA_PAGE_SIZE);

    MycMemChunk_t *chunk;
    if (mem_chunk_alloc(&chunk, arena, size) != MYC_SUCCESS) {
        return MYC_MEM_ALLOC_FAILED;
    }
    void *addr = mem_addr_from_chunk(chunk);
    return addr;
}

/* Resizes the memory chunk at 'addr' to be at least 'new_size' bytes, moves it if necessary and returns the new address. 
!!NOTE: Absolute pointers into the memory will be invalid if the chunk moves. */
void* myc_mem_arena_realloc(void *addr, uint32_t new_size)
{
    if (new_size == 0) return MYC_MEM_ALLOC_FAILED;
    new_size = MYC_QUANTIZE_UP(new_size + sizeof(MycMemChunk_t), MYC_MEM_ARENA_PAGE_SIZE);

    MycMemChunk_t *chunk = mem_chunk_from_addr(addr);    
    MycMemChunkSearchInfo_t chunk_info = mem_chunk_find(chunk);
    if (mem_chunk_resize(chunk, new_size, &chunk_info) != MYC_SUCCESS) {
        mem_chunk_free(chunk, &chunk_info); // Free first to allow overlapping allocation.
        MycMemChunk_t *new_chunk;
        if (mem_chunk_alloc(&new_chunk, chunk_info.arena->head, new_size) != MYC_SUCCESS) {
            mem_chunk_revert(chunk, &chunk_info);
            return MYC_MEM_ALLOC_FAILED;
        }
        void *new_addr = mem_addr_from_chunk(new_chunk);
        const size_t move_size = MYC_MIN(chunk->size, new_chunk->size) - sizeof(MycMemChunk_t);
        addr = memmove(new_addr, addr, move_size);
    }
    return addr;
}

/* Frees the memory chunk at 'addr', allowing it to be reused. */
void myc_mem_arena_free(void *addr)
{
    MycMemChunk_t *chunk = mem_chunk_from_addr(addr);
    MycMemChunkSearchInfo_t chunk_info = mem_chunk_find(chunk);
    mem_chunk_free(chunk, &chunk_info);
}



// === CHUNK MANAGEMENT ============================================================================================ //

static myc_err_t find_best_suitable_arena(MycMemArena_t** arena, uint32_t chunk_size);
static size_t mem_layout_find_min_suitable_bucket(const MycMemLayout_t *layout, uint32_t chunk_size);
#define UPDATE_PARENTS true
#define DONT_UPDATE_PARENTS false
static void mem_layout_update_free_sizes(MycMemLayout_t *layout, size_t bucket_idx, int32_t size_diff, bool update_parents);
static void mem_layout_merge_bucket_with_previous(MycMemLayout_t *layout, size_t bucket_idx);
static void mem_layout_split_bucket_at(MycMemLayout_t *layout, size_t bucket_idx, uint32_t split_offset);
static void mem_layout_rebuild(MycMemLayout_t *layout);
static void mem_layout_update_parent_node_count(MycMemLayout_t *layout);

static myc_err_t mem_chunk_alloc(MycMemChunk_t **new_chunk, MycMemArena_t *arena, uint32_t size)
{
    myc_err_t exit_code;
    if ((exit_code = find_best_suitable_arena(&arena, size)) != MYC_SUCCESS) {
        return exit_code;
    }

    size_t bucket_idx = mem_layout_find_min_suitable_bucket(&arena->layout, size);
    uint32_t chunk_offset = mem_layout_bucket_free_offset(&arena->layout, bucket_idx);
    mem_layout_update_free_sizes(&arena->layout, bucket_idx, (int32_t)(-size), UPDATE_PARENTS);

    MycMemChunk_t *chunk = mem_chunk_at(arena, chunk_offset);
    chunk->size = size;
    chunk->offset = chunk_offset;
    *new_chunk = chunk;
    return MYC_SUCCESS;
}

static MycMemChunkSearchInfo_t mem_chunk_find(const MycMemChunk_t *chunk)
{
    MycMemArena_t *arena = mem_chunk_get_arena(chunk);
    size_t start_idx = 0;
    size_t mid_idx;
    size_t end_idx = arena->layout.bucket_node_count;
    while (start_idx < end_idx - 1) {
        mid_idx = (start_idx + end_idx) / 2;
        if (chunk->offset < arena->layout.bucket_offsets[mid_idx]) {
            end_idx = mid_idx;
        } else {
            start_idx = mid_idx;
        }
    }

    const size_t bucket_idx = start_idx;
    MycMemChunkSearchInfo_t chunk_info = {
        .arena = arena,
        .bucket_idx = bucket_idx,
        .is_first_in_bucket = (chunk->offset == arena->layout.bucket_offsets[bucket_idx]),
        .is_last_in_bucket = (chunk->offset + chunk->size == mem_layout_bucket_free_offset(&arena->layout, bucket_idx)),
    };
    return chunk_info;
}

static myc_err_t mem_chunk_resize(MycMemChunk_t *chunk, uint32_t new_size, MycMemChunkSearchInfo_t *chunk_info)
{
    MYC_ASSERT(!(chunk_info->bucket_idx == 0 && chunk_info->is_first_in_bucket), "First chunk is internal and never resized.");
    if (new_size == chunk->size) {
        return MYC_SUCCESS;
    }

    MycMemLayout_t *layout = &chunk_info->arena->layout;
    int32_t size_diff = (int32_t)(new_size - chunk->size);
    if (chunk_info->is_last_in_bucket) {
        if (size_diff > (int32_t)mem_layout_bucket_free_size(layout, chunk_info->bucket_idx)) {
            return MYC_FAILED;
        }
        mem_layout_update_free_sizes(layout, chunk_info->bucket_idx, -size_diff, UPDATE_PARENTS);
    } else {
        if (size_diff > 0) {
            return MYC_FAILED;
        }
        const uint32_t split_offset = chunk->offset + chunk->size;
        mem_layout_split_bucket_at(layout, chunk_info->bucket_idx, split_offset);
        mem_layout_update_free_sizes(layout, chunk_info->bucket_idx, -size_diff, DONT_UPDATE_PARENTS);
        mem_layout_rebuild(layout);
    }

    chunk->size += size_diff;
    MYC_ASSERT(chunk->size == new_size, "Chunk size and new size must match after successful resize.");
    return MYC_SUCCESS;
}

static void mem_chunk_free(MycMemChunk_t *chunk, MycMemChunkSearchInfo_t *chunk_info)
{
    MYC_ASSERT(!(chunk_info->bucket_idx == 0 && chunk_info->is_first_in_bucket), "First chunk is internal and never freed.");
    bool is_chunk_valid = chunk->offset < mem_layout_bucket_free_offset(&chunk_info->arena->layout, chunk_info->bucket_idx);
    MYC_ASSERT(is_chunk_valid, "Attempt to free invalid memory chunk.");

    MycMemLayout_t *layout = &chunk_info->arena->layout;
    if (chunk_info->is_first_in_bucket && chunk_info->is_last_in_bucket) {
        const uint32_t bucket_size = mem_layout_bucket_size(layout, chunk_info->bucket_idx);
        mem_layout_merge_bucket_with_previous(layout, chunk_info->bucket_idx);
        chunk_info->bucket_idx -= 1;
        mem_layout_update_free_sizes(layout, chunk_info->bucket_idx, (int32_t)bucket_size, DONT_UPDATE_PARENTS);
        mem_layout_rebuild(layout);
    } else if (chunk_info->is_first_in_bucket) {
        layout->bucket_offsets[chunk_info->bucket_idx] += chunk->size;
        chunk_info->bucket_idx -= 1;
        mem_layout_update_free_sizes(layout, chunk_info->bucket_idx, (int32_t)chunk->size, UPDATE_PARENTS);
    } else if (chunk_info->is_last_in_bucket) {
        mem_layout_update_free_sizes(layout, chunk_info->bucket_idx, (int32_t)chunk->size, UPDATE_PARENTS);
    } else {
        const uint32_t split_offset = chunk->offset + chunk->size;
        mem_layout_split_bucket_at(layout, chunk_info->bucket_idx, split_offset);
        mem_layout_update_free_sizes(layout, chunk_info->bucket_idx, (int32_t)chunk->size, DONT_UPDATE_PARENTS);
        mem_layout_rebuild(layout);
    }
}

static void mem_chunk_revert(const MycMemChunk_t *chunk, MycMemChunkSearchInfo_t *chunk_info)
{
    MYC_ASSERT(!(chunk_info->bucket_idx == 0 && chunk_info->is_first_in_bucket), "First chunk is internal and never reverted.");

    MycMemLayout_t *layout = &chunk_info->arena->layout;
    const bool is_first_free_chunk = chunk->offset == mem_layout_bucket_free_offset(layout, chunk_info->bucket_idx);
    const bool is_last_free_chunk = chunk->offset + chunk->size == layout->bucket_offsets[chunk_info->bucket_idx + 1];
    if (is_first_free_chunk) {
        mem_layout_update_free_sizes(layout, chunk_info->bucket_idx, (int32_t)(-chunk->size), UPDATE_PARENTS);
    } else if (is_last_free_chunk) {
        mem_layout_update_free_sizes(layout, chunk_info->bucket_idx, (int32_t)(-chunk->size), UPDATE_PARENTS);
        chunk_info->bucket_idx += 1;
        layout->bucket_offsets[chunk_info->bucket_idx] -= chunk->size;
    } else {
        mem_layout_split_bucket_at(layout, chunk_info->bucket_idx, chunk->offset);
        chunk_info->bucket_idx += 1;
        mem_layout_update_free_sizes(layout, chunk_info->bucket_idx, (int32_t)(-chunk->size), DONT_UPDATE_PARENTS);
        mem_layout_rebuild(layout);
    }
}



// === LAYOUT MANAGEMENT =========================================================================================== //

static myc_err_t find_best_suitable_arena(MycMemArena_t** arena, uint32_t chunk_size)
{
    myc_err_t is_found = MYC_FAILED;
    uint32_t min_suitable_free_size = UINT32_MAX;
    for (MycMemArena_t *arena_i = *arena; arena_i != NULL; arena_i = arena_i->next) {
        const uint32_t max_free_size = arena_i->layout.max_free_sizes[0];
        if (max_free_size >= chunk_size && max_free_size <= min_suitable_free_size) {
            min_suitable_free_size = max_free_size;
            *arena = arena_i;
            is_found = MYC_SUCCESS;
        }
    }
    return is_found;
}

static size_t mem_layout_find_min_suitable_bucket(const MycMemLayout_t *layout, uint32_t chunk_size)
{
    size_t node_idx = 0;
    uint32_t min_suitable_free_size = layout->max_free_sizes[node_idx];
    while (node_idx < layout->parent_node_count) {
        const size_t start_idx = node_children_base_idx(node_idx);
        const size_t end_idx = MYC_MIN(start_idx + MYC_MEM_LAYOUT_NODE_CHILD_COUNT, mem_layout_node_count(layout));
        for (size_t child_idx = start_idx; child_idx < end_idx; ++child_idx) {
            const uint32_t max_free_size = layout->max_free_sizes[child_idx];
            if (max_free_size >= chunk_size && max_free_size <= min_suitable_free_size) {
                min_suitable_free_size = max_free_size;
                node_idx = child_idx;
            }
        }
    }
    const size_t bucket_idx = node_idx - layout->parent_node_count;
    return bucket_idx;
}

static void mem_layout_update_free_sizes(MycMemLayout_t *layout, size_t bucket_idx, int32_t size_diff, bool update_parents)
{
    size_t node_idx = bucket_idx + layout->parent_node_count;
    layout->max_free_sizes[node_idx] += size_diff;

    bool keep_updating = update_parents;
    while (node_idx > 0 && keep_updating) {
        const size_t parent_idx = node_parent_idx(node_idx);
        const size_t start_idx = node_children_base_idx(node_idx);
        const size_t end_idx = MYC_MIN(start_idx + MYC_MEM_LAYOUT_NODE_CHILD_COUNT, mem_layout_node_count(layout));
        uint32_t new_max_free_size = layout->max_free_sizes[parent_idx];
        for (size_t child_idx = start_idx; child_idx < end_idx; ++child_idx) {
            new_max_free_size = MYC_MAX(layout->max_free_sizes[child_idx], new_max_free_size);
        }
        keep_updating = (new_max_free_size != layout->max_free_sizes[parent_idx]);
        layout->max_free_sizes[parent_idx] = new_max_free_size;
        node_idx = parent_idx;
    }
}

static void mem_layout_merge_bucket_with_previous(MycMemLayout_t *layout, size_t bucket_idx)
{
    const size_t node_idx = bucket_idx + layout->parent_node_count;
    
    uint32_t *bucket = &layout->bucket_offsets[bucket_idx];
    size_t move_size = (layout->bucket_node_count - bucket_idx) * sizeof(uint32_t);
    memmove(bucket, bucket + 1, move_size);

    uint32_t *node = &layout->max_free_sizes[node_idx];
    move_size = (mem_layout_node_count(layout) - node_idx - 1) * sizeof(uint32_t);
    memmove(node, node + 1, move_size);

    layout->bucket_node_count -= 1;
    mem_layout_update_parent_node_count(layout);
}

static void mem_layout_split_bucket_at(MycMemLayout_t *layout, size_t bucket_idx, uint32_t split_offset)
{
    const size_t node_idx = bucket_idx + layout->parent_node_count;
    const uint32_t new_bucket_size = layout->bucket_offsets[bucket_idx + 1] - split_offset;

    uint32_t *bucket = &layout->bucket_offsets[bucket_idx];
    size_t move_size = (layout->bucket_node_count - bucket_idx) * sizeof(uint32_t);
    memmove(bucket + 2, bucket + 1, move_size);

    uint32_t *node = &layout->max_free_sizes[node_idx];
    move_size = (mem_layout_node_count(layout) - node_idx) * sizeof(uint32_t);
    memmove(node + 1, node, move_size);

    layout->bucket_node_count += 1;
    layout->bucket_offsets[bucket_idx + 1] = split_offset;
    layout->max_free_sizes[node_idx] = MYC_MAX((int32_t)(layout->max_free_sizes[node_idx] - new_bucket_size), 0);
    layout->max_free_sizes[node_idx + 1] = MYC_MIN(layout->max_free_sizes[node_idx + 1], new_bucket_size);
    mem_layout_update_parent_node_count(layout);
}

static void mem_layout_update_parent_node_count(MycMemLayout_t *layout)
{
    const size_t new_prarent_node_count = calc_parent_node_count(layout->bucket_node_count);
    if (new_prarent_node_count != layout->parent_node_count) {
        uint32_t *node = &layout->max_free_sizes[layout->parent_node_count];
        size_t move_offset = new_prarent_node_count - layout->parent_node_count;
        size_t move_size = layout->bucket_node_count * sizeof(uint32_t);
        memmove(node + move_offset, node, move_size);
    }
    layout->parent_node_count = new_prarent_node_count;
}

static void mem_layout_rebuild(MycMemLayout_t *layout)
{
    memset(layout->max_free_sizes, 0, layout->parent_node_count * sizeof(uint32_t));
    for (size_t node_idx = mem_layout_node_count(layout); node_idx > 0; --node_idx) {
        const size_t parent_idx = node_parent_idx(node_idx);
        layout->max_free_sizes[parent_idx] = MYC_MAX(layout->max_free_sizes[node_idx], layout->max_free_sizes[parent_idx]);
    }
}