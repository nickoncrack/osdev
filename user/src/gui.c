// gui test program

#include <string.h>
#include <stdlib.h>
#include <sys/syscall.h>


void draw_rect(uint32_t *lfb, int x0, int y0, int x1, int y1, uint32_t color, int fill) {
    // these will be drawn anyway
    memsetd((void *) (lfb + y0 * 250 + x0), color, x1 - x0);
    memsetd((void *) (lfb + y1 * 250 + x0), color, x1 - x0);

    if (!fill) {
        for (int y = y0; y < y1; y++) {
            *(lfb + y * 250 + x0) = color;
            *(lfb + y * 250 + x1) = color;
        }

        return;
    }

    for (int y = y0 + 1; y < y1; y++) {
        memsetd((void *) (lfb + y * 250 + x0), color, x1 - x0);
    }
}

uint32_t rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return (r << 18) + (g << 10) + (b << 8) + a;
}

void bflush(int x, int y) {
    asm volatile(
        "int $0x7F"
        :: "a"(SYS_BUFREADY), "b"(x), "c"(y)
    );
}

__attribute__((section(".text._start")))
void _start() {
    // request buffer
    uint32_t buf = 0;

    asm volatile(
        "int $0x7F"
        : "=a"(buf)
        : "a"(SYS_RQBUF), "b"(250), "c"(120)
    );

    draw_rect((uint32_t *) buf, 50, 10, 240, 119, rgba(0x00, 0xFF, 0xFF, 0xFF), 1);

    bflush(200, 300);

    exit(0);
}