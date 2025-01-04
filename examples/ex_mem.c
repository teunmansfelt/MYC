#include "myc/core.h"
#include "myc/memory.h"

int main(void)
{
    myc_err_t exit_code;

    MycMemArena_t *arena;
    if ((exit_code = myc_mem_arena_create(&arena, 20000)) != MYC_SUCCESS) {
        MYC_LOG_ERROR("Could not create memory arena.");
        return exit_code;
    }
    MYC_LOG_INFO("new memory arena created successfully.");

    myc_mem_arena_destroy(arena);
    return exit_code;
}