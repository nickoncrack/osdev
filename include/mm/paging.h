#pragma once

#include <int/isr.h>

#define PAGE_DIR_SIZE 1024
#define PAGE_TAB_SIZE 1024

typedef struct {
    uint32_t present  : 1;
    uint32_t rw       : 1;
    uint32_t user     : 1;
    uint32_t accessed : 1;
    uint32_t dirty    : 1;
    uint32_t unused   : 7;
    uint32_t frame    : 20;
} page_t;

typedef struct {
    page_t pages[PAGE_TAB_SIZE];
} pagetab_t;

typedef struct {
    pagetab_t *tables[PAGE_DIR_SIZE];
    uint32_t tab_phy[PAGE_DIR_SIZE];
    uint32_t addr;
} pagedir_t;

extern pagedir_t *crt_dir;
extern pagedir_t *kernel_dir;

void init_paging();
void switch_page_dir(pagedir_t *dir);
page_t *get_page(uint32_t addr, int make, pagedir_t *dir);
void pgf(regs_t *regs);
void alloc_frame(page_t *page, int kernel, int writable);
void free_frame(page_t *page);
pagedir_t *clone_dir(pagedir_t *src);
void map_memory(uint32_t addr, uint32_t vaddr, uint32_t size, pagedir_t *dir, int use_existing_phys, uint8_t user);
void unmap_memory(uint32_t vaddr, uint32_t size, pagedir_t *dir);