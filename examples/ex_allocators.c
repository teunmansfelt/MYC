#include "myc/core.h"
#include "myc/memory.h"

int main(void)
{
    myc_err_t exit_code;

    MycMemArena_t *arena;
    if ((exit_code = myc_mem_arena_create(&arena, 30000)) != MYC_SUCCESS) {
        MYC_LOG_ERROR("Could not create memory arena.");
        return exit_code;
    }

    MycMemBumpAlloc_t *bump_alloc;
    if ((exit_code = myc_mem_bump_alloc_create(&bump_alloc, arena, 4000)) != MYC_SUCCESS) {
        MYC_LOG_ERROR("Could not create bump allocator.");
        goto _exit;
    }
    MYC_LOG_INFO("new bump allocator created successfully.");
    myc_mem_arena_introspect(arena);
    
    MYC_LOG_INFO("Bump allocations with alignedment 1");
    for (size_t i = 1; i < 6; ++i) {
        MYC_LOG("addr: %p", myc_mem_bump_aligned_malloc(bump_alloc, 17 * i, 1));
    }
    MYC_LOG("Bump allocations with alignedment 2");
    for (size_t i = 1; i < 6; ++i) {
        MYC_LOG("addr: %p", myc_mem_bump_aligned_malloc(bump_alloc, 17 * i, 2));
    }
    MYC_LOG("Bump allocations with alignedment 4");
    for (size_t i = 1; i < 6; ++i) {
        MYC_LOG("addr: %p", myc_mem_bump_aligned_malloc(bump_alloc, 17 * i, 4));
    }
    MYC_LOG("Bump allocations with alignedment 8");
    for (size_t i = 1; i < 6; ++i) {
        MYC_LOG("addr: %p", myc_mem_bump_malloc(bump_alloc, 17 * i));
    }
    MYC_LOG("Bump allocations with alignedment 16");
    for (size_t i = 1; i < 6; ++i) {
        MYC_LOG("addr: %p", myc_mem_bump_aligned_malloc(bump_alloc, 17 * i, 16));
    }

_exit:
    myc_mem_arena_destroy(arena);
    return exit_code;
}