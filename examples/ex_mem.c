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
    if ((exit_code = myc_mem_arena_expand(arena, 10000)) != MYC_SUCCESS) {
        MYC_LOG_ERROR("Could not expand memory arena.");
        goto _exit;
    }
    MYC_LOG_INFO("new memory arena created successfully.");
    myc_mem_arena_introspect(arena);
    
    void *addrs[16];
    for (size_t i = 0; i < 16; ++i) {
        addrs[i] = myc_mem_arena_malloc(arena, 240 * (i + 1));
        MYC_ASSERT(addrs[i], "'myc_mem_arena_malloc' never fails in this context.");
    }
    MYC_LOG_INFO("All addresses allocated successfully.");
    myc_mem_arena_introspect(arena);

    for (size_t i = 0; i < 16; i += 3) {
        myc_mem_arena_free(addrs[i]);
    }
    MYC_LOG_INFO("Freed one thrid of the addresses.");
    myc_mem_arena_introspect(arena);

    addrs[1] = myc_mem_arena_realloc(addrs[1], 1000);
    addrs[2] = myc_mem_arena_realloc(addrs[2], 3000);
    addrs[4] = myc_mem_arena_realloc(addrs[4], 500);
    MYC_ASSERT(addrs[1] && addrs[2] && addrs[4], "'myc_mem_arena_realloc' never fails in this context.");
    addrs[5] = myc_mem_arena_realloc(addrs[5], 20000);
    MYC_ASSERT(addrs[5] == MYC_MEM_ALLOC_FAILED, "'myc_mem_arena_realloc' always fails in this context.");
    MYC_LOG_INFO("Reallocated three addresses and reverted one relocation.");
    myc_mem_arena_introspect(arena);

    myc_mem_arena_reset(arena);
    MYC_LOG_INFO("Reset the memory arena.");
    myc_mem_arena_introspect(arena);

_exit:
    myc_mem_arena_destroy(arena);
    return exit_code;
}