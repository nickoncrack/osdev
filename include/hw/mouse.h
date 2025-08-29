#pragma once

#include <stdint.h>


#define LEFT_CLICK      0x01
#define RIGHT_CLICK     0x02
#define MIDDLE_CLICK    0x04
#define BUTTON_4        0x08
#define BUTTON_5        0x10

typedef struct {
    uint8_t button;
    int8_t dx, dy, dz;
} mouse_packet_t;

void mouse_init();
