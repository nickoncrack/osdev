#pragma once

#include <stdint.h>

#define BIN_BASE_ADDR   0x40000000
#define BIN_END_ADDR    0x80000000

#define MAX_PROCESS_SIZE 0x800000
#define MAX_PROCESS ((BIN_END_ADDR - BIN_BASE_ADDR) / MAX_PROCESS_SIZE)

struct process_address_space {
    /*
        address_idx: index in "addresses",
        base address = (BASE_ADDR + i * 8 MB)
        stack top = (BASE_ADDR + (i + 1) * 8MB - 1)
        heap top = (BASE_ADDR + (i + 1) * 8MB - 128 kb)
        heap bottom = (BASE_ADDR + i * 8 MB + binary_size)
    */
    uint16_t address_idx;
    uint32_t binary_size;
};

int exec(char *path);