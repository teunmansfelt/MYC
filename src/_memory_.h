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
typedef struct _MycMemoryChunkSearchInfo MycMemChunkSearch_t;

static inline size_t calc_mem_layout_parent_node_count(size_t bucket_node_count) {
    return (bucket_node_count + MYC_MEM_LAYOUT_NODE_CHILD_COUNT - 3) / (MYC_MEM_LAYOUT_NODE_CHILD_COUNT - 1);
}

static inline size_t calc_mem_layout_free_size_node_count(size_t bucket_node_count) {
    return bucket_node_count + calc_mem_layout_parent_node_count(bucket_node_count);
}

typedef struct _MycMemoryLayout {
    size_t bucket_node_count;
    size_t parent_node_count;
    uint32_t *bucket_offsets;
    uint32_t *max_free_sizes;
} MycMemLayout_t;

typedef struct _MycMemoryArena {
    size_t size;
    size_t internal_size;
    MycMemLayout_t layout;
    MycMemArena_t *head;
    MycMemArena_t *next;
} MycMemArena_t;


#endif // _MYC_MEMORY_INTENRAL_H_