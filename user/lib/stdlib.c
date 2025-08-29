#include <stdlib.h>
#include <sys/syscall.h>

__attribute__((malloc))
void *malloc(uint32_t size) {
    uint32_t addr = 0;

    asm volatile(
        "int $0x7F\n"
        : "=a"(addr)
        : "a"(SYS_MALLOC), "b"(size)
        : "memory"
    );

    return (void *) addr;
}

void free(void *ptr) {
    if (!ptr) return;

    asm volatile(
        "int $0x7F"
        :: "a"(SYS_FREE), "b"((uint32_t) ptr)
        : "memory"
    );

    return;
}

void exit(uint32_t ret) {
    asm volatile("int $0x7F" :: "a"(SYS_EXIT), "b"(ret));

    __builtin_unreachable();
}