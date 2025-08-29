#include <video/vbe.h>
#include <sys/syscall.h>

void get_display_info(struct vbe_mode_info *vbe_info) {
    asm volatile(
        "int $0x7F"
        :: "a"(SYS_GETVBEINFO), "b"((uint32_t) vbe_info)
        : "memory"
    );

    asm volatile("" ::: "memory");
}