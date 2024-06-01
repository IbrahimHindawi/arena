#include <stdio.h>
#include <windows.h>
#include "core.h"
#include <math.h>

#define pages_max 10

#ifdef OPT
#   define assert(expr)
#else
#   define assert(expr) if (!(expr)) { __debugbreak(); }
#endif

u64 div_ceil(u64 a, u64 b) {
    double r = ceil((double)a / (double)b);
    return (u64)r;
}

i8 is_power_of_two(u64 x) {
    return (x & (x - 1)) == 0;
}

u64 align_forward(u64 ptr, size_t align) {
    u64 p, a, modulo;
    assert(is_power_of_two(align));
    
    p = ptr;
    a = (u64)align;
    modulo = p & (a - 1);
    if (modulo != 0) {
        p += a - modulo;
    }
    return p;
}

structdef(Arena) {
    u8 *stack_base_memory;
    u64 stack_alloc_pos;
    u64 page_size;
    u64 pages_used;
};

Arena *arenaAlloc(void) {
    SYSTEM_INFO system_info = {0};
    GetSystemInfo(&system_info);
    u64 page_size = system_info.dwPageSize;
    printf(TEXT("System page size = %lld.\n"), page_size);
    u64 pages_used = 0;
    u8 *memory;
    if (NULL == (memory = VirtualAlloc(NULL, page_size * pages_max, MEM_RESERVE, PAGE_NOACCESS))) {
        printf("Log::Arena::Error::VirtualAlloc reserve failure.\n");
        ExitProcess(GetLastError());
    }
    pages_used += 1;
    if (NULL == VirtualAlloc(memory, page_size * pages_used, MEM_COMMIT, PAGE_READWRITE)) {
        printf("Log::Arena::Error::VirtualAlloc commit failure.\n");
        ExitProcess(GetLastError());
    }
    Arena *arena = (Arena *)memory;
    arena->stack_base_memory = memory;
    arena->stack_alloc_pos += sizeof(*arena);
    arena->page_size = page_size;
    arena->pages_used = pages_used;
    printf("Log::Arena::%lld bytes Allocated of 1 page.\n", page_size);
    return arena;
}

void arenaRelease(Arena *arena) {
    VirtualFree(arena->stack_base_memory, 0, MEM_RELEASE);
    printf("Log::Arena::Released memory.\n");
}

void *arenaPush(Arena *arena, u64 size) {
    if (arena->stack_alloc_pos + size > arena->page_size * arena->pages_used) {
        u64 required_page_count = div_ceil(size, arena->page_size);
        assert(required_page_count > 0);
        u64 next_page_start_pos = (u64)arena->stack_base_memory + arena->page_size * arena->pages_used;
        if (NULL == VirtualAlloc((void *)next_page_start_pos, arena->page_size * required_page_count, MEM_COMMIT, PAGE_READWRITE)) {
            printf("Log::Arena::Error::VirtualAlloc commit failure.\n");
            ExitProcess(GetLastError());
        }
        arena->pages_used += required_page_count;
    }
    void *result = (void *)align_forward((u64)arena->stack_base_memory + arena->stack_alloc_pos, 16);
    arena->stack_alloc_pos += size;
    return result;
}

void *arenaPushZero(Arena *arena, u64 size) {
    void *result = arena->stack_base_memory + arena->stack_alloc_pos;
    for (i32 i = 0; i < size; ++i) {
        ((u8 *)result)[i] = 0; 
    }
    arena->stack_alloc_pos += size;
    return result;
}

void arenaPop(Arena *arena, u64 size) {
    arena->stack_alloc_pos -= size;
}

u64 arenaGetPos(Arena *arena) {
    return arena->stack_alloc_pos;
}

void arenaSetPos(Arena *arena, u64 pos) {
    arena->stack_alloc_pos = pos;
}

void arenaClear(Arena *arena) { }

#define arenaPushArray(arena, type, count) arenaPush((arena), sizeof(type) * (count))
#define arenaPushArrayZero(arena, type, count) arenaPushZero((arena), sizeof(type) * (count))
#define arenaPushStruct(arena, type) arenaPushArray(arena, type, 1)
#define arenaPushStructZero(arena, type) arenaPushArrayZero(arena, type, 1)

structdef(Payload) {
    i32 x;
    i32 y;
    i32 z;
    i32 w;
};

#define N 1000

int main() {
    Arena *arena = arenaAlloc();
    printf("Log::Arena::Size of Arena struct = %lld.\n", sizeof(*arena));
    printf("Log::Arena::Current arena position = %lld.\n", arenaGetPos(arena));
    u64 stack_pos_save = arenaGetPos(arena);
    Payload *payload = arenaPush(arena, sizeof(*payload) * N);
    printf("Log::Arena::Current arena position = %lld.\n", arenaGetPos(arena));
    u64 last_byte_in_page = (u64)arena->stack_base_memory + arena->page_size * arena->pages_used;
    printf("Log::Arena::Last byte in page: %llx\n", last_byte_in_page);
    for (i32 i = 0; i < N; ++i) {
        payload[i].x = 0xDEADBEEF;
        payload[i].y = 0xBABEFEAD;
        payload[i].z = 0xCAFEBABE;
        payload[i].w = 0xB105F00D;
    }
    arenaSetPos(arena, stack_pos_save);
    printf("Log::Arena::Current arena position = %lld.\n", arenaGetPos(arena));
    arenaPushArrayZero(arena, Payload, N);
    printf("Log::Arena::Current arena position = %lld.\n", arenaGetPos(arena));
    // arenaPop(arena, sizeof(*payload));
    // arenaPushStructZero(arena, Payload);
    arenaSetPos(arena, stack_pos_save);
    printf("Log::Arena::Current arena position = %lld.\n", arenaGetPos(arena));
    // ptrdiff_t diff = arena->stack_alloc_pos + (u64)arena->stack_base_memory;
    // printf("Log::Arena::pointer diff = %tx.\n", diff);
    arenaRelease(arena);
    return 0;
}
