#pragma once

#include <stdint.h>

void putpixel(int x, int y, uint32_t color);
void draw_rect(int x, int y, int w, int h, uint32_t color, int fill);
void draw_line(int x0, int y0, int x1, int y1, uint32_t color);