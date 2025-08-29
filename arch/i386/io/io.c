// general i/o functions

#include <asm/io.h>

uint32_t read_eflags() {
    uint32_t eflags;
    asm volatile(
        "pushfl\n"
        "popl %0"
        : "=r"(eflags)
    );

    return eflags;
}

void write_eflags(uint32_t eflags) {
    asm volatile(
        "pushl %0\n"
        "popfl"
        :: "r"(eflags)
    );
}