#pragma once

#include <stdint.h>

#define DEBUG

// code macros
#define PACKED __attribute__((packed))

#define NULL ((void *) 0)

#define VESA_MODE

#define PAGE_SIZE 0x1000

#define BIN_BASE_ADDR   0x40000000
#define BIN_END_ADDR    0x80000000

#define MAX_PROCESS_SIZE 0x800000 // 8 mib per process
#define MAX_PROCESS ((BIN_END_ADDR - BIN_BASE_ADDR) / MAX_PROCESS_SIZE)

#define LFB_PHYS_ADDR   0xFD000000
#define LFB_VADDR       0xD0000000 
#define LFB_SIZE        0x00200000 // 2 mib, enough for 800x600x32

#define PROCESS_STACK_SIZE 0x20000 // 128 kb stack size