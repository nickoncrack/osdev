#include <ssfn.h>
#include <string.h>

#include <video/vbe.h>

uint32_t *lfb;
uint32_t width, height, bpp, pitch;

void putpixel(int x, int y, uint32_t color) {
    *(lfb + y * (pitch / 4) + x) = color;
}

void init_vbe(struct vbe_mode_info *vbe_info) {
    lfb = (uint32_t *) LFB_VADDR;

    width = vbe_info->width;
    height = vbe_info->height;
    bpp = vbe_info->bpp;
    pitch = vbe_info->pitch;
}

void vesa_puts(char *s) {
    if (strcmp(s, "\033[H\033[J")) {
        memset((uint8_t *) lfb, 0, width * height * (bpp / 8));
        ssfn_dst.x = 1;
        ssfn_dst.y = 0;
        return;
    }

    while (*s != '\0') {
        vesa_putc(*s);
        s++;
    }
}

static void vesa_scroll() {
    int offset = 17 * pitch;
    int size = SCREEN_HEIGHT * pitch;

    memmove((uint8_t *) lfb, (uint8_t *) lfb + offset, size - offset);
    memset((uint8_t *) lfb + (size - offset), 0, offset);
}

void vesa_putc(char c) {
    if (ssfn_dst.x + 9 >= SCREEN_WIDTH) {
        ssfn_dst.y += 17;
        ssfn_dst.x = 1;
    }

    if (ssfn_dst.y + 17 > SCREEN_HEIGHT) {
        vesa_scroll();
        ssfn_dst.y -= 17;
    }

    // when *s = '\n', ssfn_putc prints a weird unicode instead of a newline
    if (c == '\n') {
        ssfn_dst.y += 17; // 17 looks perfect for me. the pixels i mean, i am not drake
        ssfn_dst.x = 1;
    } else if (c == '\b') {
        if (ssfn_dst.x - 9 <= 1 || ssfn_dst.x == 3 * 9 + 1) {
            return;
        }

        ssfn_dst.x -= 9;

        for (int y = 0; y < 17; y++) {
            for (int x = 0; x < 9; x++) {
                putpixel(ssfn_dst.x + x, ssfn_dst.y + y, 0x000000);
            }
        }
    } else {
        ssfn_putc(c);
    }
}