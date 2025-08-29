#include <assert.h>
#include <string.h>

#include <asm/io.h>
#include <mm/kheap.h>
#include <mm/paging.h>
#include <video/vbe.h>
#include <video/vga.h>

#define INDEX_FROM_BIT(a) (a / 32)
#define OFF_FROM_BIT(a) (a % 32)

extern void copy_page_physical(uint32_t, uint32_t);
extern heap_t *kheap;

pagedir_t *kernel_dir = 0;
pagedir_t *crt_dir = 0;

uint32_t *frames;
uint32_t nframes;

extern uint32_t placement_addr;

static void set_frame(uint32_t frame) {
    uint32_t idx = INDEX_FROM_BIT(frame);
    uint32_t off = OFF_FROM_BIT(frame);
    frames[idx] |= (0x1 << off);
}

static void clear_frame(uint32_t addr) {
    uint32_t frame = addr / 0x1000;
    uint32_t idx = INDEX_FROM_BIT(frame);
    uint32_t off = OFF_FROM_BIT(frame);
    frames[idx] &= ~(0x1 << off);
}

static uint32_t test_frame(uint32_t addr) {
    uint32_t frame = addr / 0x1000;
    uint32_t idx = INDEX_FROM_BIT(frame);
    uint32_t off = OFF_FROM_BIT(frame);
    return (frames[idx] & (0x1 << off));
}

static uint32_t first_frame() {
    uint32_t i, j;
    for (i = 0; i < INDEX_FROM_BIT(nframes); i++) {
        if (frames[i] != 0xFFFFFFFF) {
            for (j = 0; j < 32; j++) {
                uint32_t test = 0x1 << j;
                if (!(frames[i] & test)) {
                    return i * 4 * 8 + j;
                }
            }
        }
    }
}


void alloc_frame(page_t *page, int kernel, int writable) {
    if (page->frame != 0) {
        return;
    } else {
        uint32_t idx = first_frame();
        assert(idx != (uint32_t) - 1);
        set_frame(idx);
        page->present = 1;
        page->rw = (writable) ? 1 : 0;
        page->user = (kernel) ? 0 : 1;
        page->frame = idx;
    }
}

void free_frame(page_t *page) {
    uint32_t frame;
    if (!(frame = page->frame)) {
        return;
    } else {
        clear_frame(frame);
        page->frame = 0x0;
    }
}


void map_memory(uint32_t addr, uint32_t vaddr, uint32_t size, pagedir_t *dir, int use_existing_phys, uint8_t user) {
    if (size < 0x1000) {
        size = 0x1000;
    }

    uint32_t npages = size / 0x1000;

    for (uint32_t i = 0; i < npages; i++) {
        uint32_t page_addr = addr + i * 0x1000;
        uint32_t page_vaddr = vaddr + i * 0x1000;

        page_t *page = get_page(page_vaddr, 1, dir);

        if (!page->present) {
            if (use_existing_phys) {
                page->present = 1;
                page->rw = 1;
                page->user = user;
                page->frame = page_addr / 0x1000;
            } else {
                alloc_frame(page, 1, 1);
            }
        }
    }

    switch_page_dir(dir);
}

void unmap_memory(uint32_t vaddr, uint32_t size, pagedir_t *dir) {
    if (size < 0x1000) {
        size = 0x1000;
    }

    uint32_t npages = size / 0x1000;

    for (uint32_t i = 0; i < npages; i++) {
        uint32_t page_vaddr = vaddr + i * 0x1000;

        page_t *page = get_page(page_vaddr, 0, dir);

        page->present = 0;
        page->rw = 0;
        page->user = 0;

        clear_frame(page->frame);
        page->frame = 0;
    }

    switch_page_dir(dir);
}

void init_paging() {
    uint32_t mem_end = 0x2000000;

    nframes = mem_end / 0x1000;
    frames = (uint32_t *) kmalloc(INDEX_FROM_BIT(nframes));
    memset(frames, 0, INDEX_FROM_BIT(nframes));

    uint32_t addr;
    kernel_dir = (pagedir_t *) kmalloc_a(sizeof(pagedir_t));
    memset(kernel_dir, 0, sizeof(pagedir_t));
    kernel_dir->addr = (uint32_t) kernel_dir->tab_phy;

    int i = 0;
    while (i < 0x400000) {
        alloc_frame(get_page(i, 1, kernel_dir), 0, 0);
        i += 0x1000;
    }

    for (i = KHEAP_START; i < KHEAP_START + KHEAP_INITIAL_SZ; i += 0x1000) {
        alloc_frame(get_page(i, 1, kernel_dir), 0, 0);
    }

    map_memory(LFB_PHYS_ADDR, LFB_VADDR, LFB_SIZE, kernel_dir, 1, 1);

    register_interrupt_handler(0x0E, pgf);
    switch_page_dir(kernel_dir);

    kheap = mkheap(KHEAP_START, KHEAP_START + KHEAP_INITIAL_SZ, 0xCFFFF000, 0, 0);

    serial_printf("Kernel heap initialized at 0x%x\n", KHEAP_START);
}

void switch_page_dir(pagedir_t *dir) {
    crt_dir = dir;
    if (dir->addr == 0) {
        asm volatile("cli");
        uint32_t caller = (uint32_t) __builtin_return_address(0);
        serial_printf("Caller = 0x%x\n", caller);

        for(;;);
    }

    asm volatile("mov %0, %%cr3" :: "r"(dir->addr));

    uint32_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;
    asm volatile("mov %0, %%cr0" :: "r"(cr0));
}

page_t *get_page(uint32_t addr, int make, pagedir_t *dir) {
    addr /= 0x1000;
    uint32_t tab_idx = addr / 1024;
    if (dir->tables[tab_idx]) {
        return &dir->tables[tab_idx]->pages[addr % 1024];
    } else if (make) {
        uint32_t tmp = 0;
        dir->tables[tab_idx] = (pagetab_t *) kmalloc_ap(sizeof(pagetab_t), &tmp);
        memset(dir->tables[tab_idx], 0, 0x1000);
        dir->tab_phy[tab_idx] = tmp | 0x7;
        return &dir->tables[tab_idx]->pages[addr % 1024];
    } else {
        return 0;
    }
}

// #PF handler
void pgf(regs_t *regs) {
    asm volatile("cli");
    
    uint32_t addr; // fault address
    asm volatile("mov %%cr2, %0" : "=r" (addr));

    serial_printf("Unhandled page fault at 0x%x, err = %d\n", addr, regs->err_code);

    while (1) {
        asm volatile("hlt");
    }
}

static pagetab_t *clone_table(pagetab_t *src, uint32_t *addr) {
    pagetab_t *tab = (pagetab_t *) kmalloc_ap(sizeof(pagetab_t), addr);
    memset(tab, 0, sizeof(pagedir_t));

    for (int i = 0; i < 1024; i++) {
        if (src->pages[i].frame) {
            alloc_frame(&tab->pages[i], 0, 0);

            if (src->pages[i].present)  tab->pages[i].present = 1;
            if (src->pages[i].rw)       tab->pages[i].rw = 1;
            if (src->pages[i].user)     tab->pages[i].user = 1;
            if (src->pages[i].accessed) tab->pages[i].accessed = 1;
            if (src->pages[i].dirty)    tab->pages[i].dirty = 1;

            copy_page_physical(src->pages[i].frame * 0x1000, tab->pages[i].frame * 0x1000);
        }
    }

    return tab;
}

pagedir_t *clone_dir(pagedir_t *src) {
    uint32_t phy;
    pagedir_t *dir = (pagedir_t *) kmalloc_ap(sizeof(pagedir_t), &phy);
    memset(dir, 0, sizeof(pagedir_t));

    uint32_t off = (uint32_t) dir->tab_phy - (uint32_t) dir;
    dir->addr = phy + off;

    for (int i = 0; i < 1024; i++) {
        if (!src->tables[i]) {
            continue;
        }

        if (kernel_dir->tables[i] == src->tables[i]) {
            dir->tables[i] = src->tables[i];
            dir->tab_phy[i] = src->tab_phy[i];
        } else {
            uint32_t phy;
            dir->tables[i] = clone_table(src->tables[i], &phy);
            dir->tab_phy[i] = phy | 0x07;
        }
    }

    return dir;
}
