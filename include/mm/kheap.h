#pragma once

#include <common.h>
#include <ordered_array.h>


#define KHEAP_START      0xC0000000
#define KHEAP_INITIAL_SZ 0x100000
#define HEAP_INDEX_SZ    0x20000
#define HEAP_MAGIC       0x69694200
#define HEAP_MIN_SZ      0x70000

typedef struct {
    uint32_t magic;
    uint8_t hole;
    uint32_t size;
} header_t;

typedef struct {
    uint32_t magic;
    header_t *head;
} footer_t;

typedef struct {
    ordered_array_t index;
    uint32_t start;
    uint32_t end;
    uint32_t max;
    uint8_t supervisor;
    uint8_t ro;
} heap_t;

uint32_t kmalloc_int(uint32_t sz, int align, uint32_t *phy);
uint32_t kmalloc_a(uint32_t sz);
uint32_t kmalloc_p(uint32_t sz, uint32_t *phy);
uint32_t kmalloc_ap(uint32_t sz, uint32_t *phy);
uint32_t kmalloc(uint32_t sz);

heap_t *mkheap(uint32_t start, uint32_t end, uint32_t max, uint8_t supervisor, uint8_t ro);
void *alloc(uint32_t size, uint8_t align, heap_t *heap);

void free(void *p, heap_t *heap);
void kfree(void *p);

void *kcalloc(uint32_t nmemb, uint32_t sz);