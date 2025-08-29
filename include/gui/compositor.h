#pragma once

#include <common.h>

#define BUFFER(n) (get_task(n)->addr->buffer_start)

typedef struct buffer_queue {
    uint32_t pid;
    uint16_t x, y;
    struct buffer_queue *next;
} buffer_queue_t;

void add_buffer_to_queue(uint32_t pid, uint16_t x, uint16_t y);
void compositor();