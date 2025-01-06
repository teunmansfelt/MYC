#ifndef _MYC_MEMORY_INTENRAL_H_
#define _MYC_MEMORY_INTENRAL_H_

#include "myc/core.h"

#define MYC_MEM_ARENA_PAGE_SIZE 256
_Static_assert(MYC_IS_POWER_OFF_TWO(MYC_MEM_ARENA_PAGE_SIZE), "Memory arena page size must be a power of two.");
#define MYC_MEM_LAYOUT_NODE_CHILD_COUNT 8
#define MYC_MEM_ARENA_SIZE_MAX (size_t)UINT32_MAX
#define MYC_MEM_ALLOC_FAILED (void*)0


typedef struct _MycMemoryArena MycMemArena_t;
typedef struct _MycMemoryLayout MycMemLayout_t;
typedef struct _MycMemoryChunkHeader MycMemChunk_t;
typedef struct _MycMemoryChunkSearchInfo MycMemChunkSearchInfo_t;

static inline size_t calc_parent_node_count(size_t bucket_node_count) {
    return (bucket_node_count + MYC_MEM_LAYOUT_NODE_CHILD_COUNT - 3) / (MYC_MEM_LAYOUT_NODE_CHILD_COUNT - 1);
}

static inline size_t calc_free_size_node_count(size_t bucket_node_count) {
    return bucket_node_count + calc_parent_node_count(bucket_node_count);
}

static inline size_t node_parent_idx(size_t node_idx) {
    return (node_idx - 1) / MYC_MEM_LAYOUT_NODE_CHILD_COUNT;
}

static inline size_t node_children_base_idx(size_t node_idx) {
    return (node_idx * MYC_MEM_LAYOUT_NODE_CHILD_COUNT) + 1;
}

typedef struct _MycMemoryLayout {
    size_t bucket_node_count;
    size_t parent_node_count;
    uint32_t *bucket_offsets;
    uint32_t *max_free_sizes;
} MycMemLayout_t;

static inline size_t mem_layout_node_count(const MycMemLayout_t *layout) {
    return layout->bucket_node_count + layout->parent_node_count;
}

static inline uint32_t mem_layout_bucket_size(const MycMemLayout_t *layout, size_t bucket_idx) {
    return layout->bucket_offsets[bucket_idx + 1] - layout->bucket_offsets[bucket_idx];
}

static inline uint32_t mem_layout_bucket_free_size(const MycMemLayout_t *layout, size_t bucket_idx) {
    return layout->max_free_sizes[bucket_idx + layout->parent_node_count];
}

static inline uint32_t mem_layout_bucket_size_used(const MycMemLayout_t *layout, size_t bucket_idx) {
    return mem_layout_bucket_size(layout, bucket_idx) - mem_layout_bucket_free_size(layout, bucket_idx);
}

static inline uint32_t mem_layout_bucket_free_offset(const MycMemLayout_t *layout, size_t bucket_idx) {
    return layout->bucket_offsets[bucket_idx + 1] - mem_layout_bucket_free_size(layout, bucket_idx);
}

typedef struct _MycMemoryArena {
    size_t size;
    size_t internal_size;
    MycMemLayout_t layout;
    MycMemArena_t *head;
    MycMemArena_t *next;
} MycMemArena_t;

typedef struct _MycMemoryChunkHeader {
    uint32_t size;
    uint32_t offset;
} MycMemChunk_t;

static inline MycMemChunk_t* mem_chunk_at(const MycMemArena_t *arena, uint32_t offset) {
    return (void*)arena + offset;
}

static inline MycMemArena_t* mem_chunk_get_arena(const MycMemChunk_t* chunk) {
    return (void*)chunk - chunk->offset;
}

static inline MycMemChunk_t* mem_chunk_from_addr(void *addr) {
    return addr - sizeof(MycMemChunk_t);
}

static inline void* mem_addr_from_chunk(MycMemChunk_t *chunk) {
    return (void*)chunk + sizeof(MycMemChunk_t);
}

typedef struct _MycMemoryChunkSearchInfo {
    MycMemArena_t *arena;
    size_t bucket_idx;
    bool is_first_in_bucket;
    bool is_last_in_bucket;
} MycMemChunkSearchInfo_t;




typedef struct _MycMemBumpAllocator MycMemBumpAlloc_t;

typedef struct _MycMemBumpAllocator {
    uint32_t capacity;
    uint32_t size_used;
    MycMemBumpAlloc_t *current;
    MycMemBumpAlloc_t *next;
} MycMemBumpAlloc_t;

#endif // _MYC_MEMORY_INTENRAL_H_