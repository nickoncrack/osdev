#include <asm/io.h>

_Bool cpuid_support = 0;

_Bool is_cpuid_supported() {
    uint32_t eflags = read_eflags();

    write_eflags(eflags ^ (1 << 21)); // toggle id bit
    uint32_t new_eflags = read_eflags();

    // restore original eflags
    write_eflags(eflags);

    // if the id bit changed then cpuid is supported
    cpuid_support = (new_eflags & (1 << 21)) != (eflags & (1 << 21));

    return cpuid_support;
}

void __cpuid(uint32_t leaf, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx) {
    asm volatile(
        "cpuid"
        : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
        : "a"(leaf)
        : "memory"
    );
}