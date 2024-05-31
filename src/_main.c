#include <stdio.h>
#include <windows.h>
#include "core.h"

u8 *stack_memory = NULL;
u64 stack_alloc_pos = 0;
u64 restore_stack_alloc_pos = 0;

int main() {
    if (NULL == (stack_memory = VirtualAlloc(stack_memory, 1024 * 8, MEM_COMMIT, PAGE_READWRITE))) {
        printf("ERROR::MEM ALLOC FAILURE\n");
    }
    void *ptr = stack_memory + stack_alloc_pos;
    // alloc
    stack_alloc_pos += 64;
    // dealloc
    stack_alloc_pos -= 64;
    // stack
    restore_stack_alloc_pos = stack_alloc_pos;
    // suballoc
    // ...
    stack_alloc_pos = restore_stack_alloc_pos;
    return 0;
}
