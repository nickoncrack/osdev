#include <assert.h>
#include <string.h>

#include <mm/kheap.h>
#include <mm/paging.h>

heap_t *kheap;
heap_t *uheap;

extern uint32_t end;
uint32_t placement_addr = (uint32_t) &end;


static int find_smallest_hole(uint32_t size, uint8_t align, heap_t *heap) {
    uint32_t iter = 0;

    while (iter < heap->index.size) {
        header_t *head = (header_t *) lookup_oarr(iter, &heap->index);

        if (align > 0) {
            uint32_t pos = (uint32_t) head;
            int off = 0;

            if (((pos + sizeof(header_t)) & 0xFFFFF000) != 0) {
                off = 0x1000 - (pos + sizeof(header_t)) % 0x1000;
            }
            int hole_size = head->size - off;
            if (hole_size >= size) {
                break;
            }
        } else if (head->size >= size) {
            break;
        }

        iter++;
    }

    if (iter == heap->index.size) {
        return -1;
    }
    return iter;
}

//header less than
static char head_lt(void *a, void *b) {
    return (((header_t *) a)->size < ((header_t *) b)->size) ? 1 : 0;
}

static void expand(uint32_t size, heap_t *heap) {
    assert(size > heap->end - heap->start);

    if ((size & 0xFFFFF000) != 0) {
        size &= 0xFFFFF000;
        size += 0x1000;
    }
    assert(heap->start + size <= heap->max);

    uint32_t old = heap->end - heap->start;
    uint32_t i = old;
    while (i < size) {
        alloc_frame(get_page(heap->start + i, 1, kernel_dir), (heap->supervisor) ? 1 : 0, (heap->ro) ? 0 : 1);
        i += 0x1000;
    }

    heap->end = heap->start + size;
}

static uint32_t contract(uint32_t size, heap_t *heap) {
    assert(size < heap->end - heap->start);

    if (size & 0x1000) {
        size &= 0x1000;
        size += 0x1000;
    }

    if (size < HEAP_MIN_SZ) {
        size = HEAP_MIN_SZ;
    }
    uint32_t old = heap->end - heap->start;
    uint32_t i = old - 0x1000;
    while (size < i) {
        free_frame(get_page(heap->start + i, 0, kernel_dir));
        i -= 0x1000;
    }

    heap->end = heap->start + size;
    return size;
}

uint32_t kmalloc_int(uint32_t sz, int align, uint32_t *phy) {
    if (kheap != 0) {
        void *addr = alloc(sz, (uint8_t) align, kheap);
        if (phy != 0) {
            page_t *page = get_page((uint32_t) addr, 0, kernel_dir);
            *phy = page->frame * 0x1000 + ((uint32_t) addr & 0xFFF);
        }
        return (uint32_t) addr;
    } else {
        if (align == 1 && (placement_addr & 0xFFFFF000)) {
            placement_addr &= 0xFFFFF000;
            placement_addr += 0x1000;
        }
        if (phy) {
            *phy = placement_addr;
        }
        uint32_t tmp = placement_addr;
        placement_addr += sz;
        return tmp;
    }
}

uint32_t kmalloc_a(uint32_t sz) {
    return kmalloc_int(sz, 1, 0);
}

uint32_t kmalloc_p(uint32_t sz, uint32_t *phy) {
    return kmalloc_int(sz, 0, phy);
}

uint32_t kmalloc_ap(uint32_t sz, uint32_t *phy) {
    return kmalloc_int(sz, 1, phy);
}

uint32_t kmalloc(uint32_t sz) {
    return kmalloc_int(sz, 0, 0);
}

void *kcalloc(uint32_t nmemb, uint32_t sz) {
    void *ptr = (void *) kmalloc(nmemb * sz);
    if (ptr == 0) {
        return ptr;
    }

    memset(ptr, 0, nmemb * sz);
    return ptr;
}

heap_t *mkheap(uint32_t start, uint32_t end, uint32_t max, uint8_t supervisor, uint8_t ro) {
    heap_t *heap = (heap_t *) kmalloc(sizeof(heap_t));

    assert(start % 0x1000 == 0);
    assert(end % 0x1000 == 0);

    heap->index = place_oarr((void *) start, HEAP_INDEX_SZ, &head_lt);
    start += sizeof(type_t) * HEAP_INDEX_SZ;

    if ((start & 0xFFFFF000) != 0) {
        start &= 0xFFFFF000;
        start += 0x1000;
    }

    heap->start      = start;
    heap->end        = end;
    heap->max        = max;
    heap->supervisor = supervisor;
    heap->ro         = ro;

    header_t *hole = (header_t *) start;
    hole->size = end - start;
    hole->magic = HEAP_MAGIC;
    hole->hole = 1;
    ins_oarr((void *) hole, &heap->index);

    return heap;
}


void *alloc(uint32_t size, uint8_t align, heap_t *heap) {
    uint32_t new_size = size + sizeof(header_t) + sizeof(footer_t);
    int iter = find_smallest_hole(new_size, align, heap);

    if (iter == -1) {
        uint32_t old = heap->end - heap->start;
        uint32_t old_end = heap->end;

        expand(old + new_size, heap);
        uint32_t new = heap->end - heap->start;

        iter = 0;
        uint32_t idx = -1;
        uint32_t val = 0x0;

        while (iter < heap->index.size) {
            uint32_t tmp = (uint32_t) lookup_oarr(iter, &heap->index);
            if (tmp > val) {
                val = tmp;
                idx = iter;
            }
            iter++;
        }

        if (idx == -1) {
            header_t *head = (header_t *) old_end;
            head->magic = HEAP_MAGIC;
            head->size = new - old;
            head->hole = 1;
            
            footer_t *foot = (footer_t *) (old_end + head->size - sizeof(footer_t));
            foot->magic = HEAP_MAGIC;
            foot->head = head;

            ins_oarr((void *) head, &heap->index);
        } else {
            header_t *head = lookup_oarr(idx, &heap->index);
            head->size = new - old;

            footer_t *foot = (footer_t *) ((uint32_t) head + head->size - sizeof(footer_t));
            foot->magic = HEAP_MAGIC;
            foot->head = head;
        }

        return alloc(size, align, heap);
    }

    header_t *orig_head = (header_t *) lookup_oarr(iter, &heap->index);
    uint32_t orig_pos = (uint32_t) orig_head;
    uint32_t orig_sz = orig_head->size;

    if (orig_sz - new_size < sizeof(header_t) + sizeof(footer_t)) {
        size += orig_sz - new_size;
        new_size = orig_sz;
    }

    if (align && orig_pos & 0xFFFFF000) {
        uint32_t new_pos = orig_pos + 0x1000 - (orig_pos & 0xFFF) - sizeof(header_t);
        
        header_t *head = (header_t *) orig_pos;
        head->size = 0x1000;
        head->magic = HEAP_MAGIC;
        head->hole = 1;

        footer_t *foot = (footer_t *) ((uint32_t) new_pos - sizeof(footer_t));
        foot->magic = HEAP_MAGIC;
        foot->head = head;
        orig_pos = new_pos;
        orig_sz -= head->size;
    } else {
        rm_oarr(iter, &heap->index);
    }

    header_t *blk = (header_t *) orig_pos;
    blk->magic = HEAP_MAGIC;
    blk->hole = 0;
    blk->size = new_size;

    footer_t *blk_footer = (footer_t *) (orig_pos + sizeof(header_t) + size);
    blk_footer->magic = HEAP_MAGIC;
    blk_footer->head = blk;

    if (orig_sz - new_size > 0) {
        header_t *head = (header_t *) (orig_pos + sizeof(header_t) + size + sizeof(footer_t));
        head->magic = HEAP_MAGIC;
        head->hole = 1;
        head->size = orig_sz - new_size;

        footer_t *foot = (footer_t *) ((uint32_t) head + orig_sz - new_size - sizeof(footer_t));
        if ((uint32_t) foot < heap->end) {
            foot->magic = HEAP_MAGIC;
            foot->head = head;
        }

        ins_oarr((void *) head, &heap->index);
    }

    return (void *) ((uint32_t) blk + sizeof(header_t));
}

void free(void *p, heap_t *heap) {
    if (p == 0) {
        return;
    }

    header_t *head = (header_t *) ((uint32_t)p - sizeof(header_t));
    footer_t *foot = (footer_t *) ((uint32_t)head + head->size - sizeof(footer_t));

    assert(head->magic == HEAP_MAGIC);
    assert(foot->magic == HEAP_MAGIC);

    head->hole = 1;
    char do_add = 1;

    footer_t *testf = (footer_t *) ((uint32_t) head - sizeof(footer_t));
    if (testf->magic == HEAP_MAGIC && testf->head->hole == 1) {
        uint32_t cache = head->size;
        head = testf->head;
        foot->head = head;
        head->size += cache;
        do_add = 0;
    }

    header_t *testh = (header_t *) ((uint32_t) foot + sizeof(footer_t));
    if (testh->magic == HEAP_MAGIC && testh->hole == 1) {
        head->size += testh->size;
        testf = (footer_t *) ((uint32_t) testh + testh->size - sizeof(footer_t));

        foot = testf;
        uint32_t iter = 0;
        while ((iter < heap->index.size) && (lookup_oarr(iter, &heap->index) != (void *) testh)) {
            iter++;
        }

        assert(iter < heap->index.size);
        rm_oarr(iter, &heap->index);
    }

    if ((uint32_t) foot + sizeof(footer_t) == heap->end) {
        uint32_t old = heap->end - heap->start;
        uint32_t new = contract((uint32_t) head - heap->start, heap);

        if (head->size - (old - new) > 0) {
            head->size -= old - new;
            foot = (footer_t *) ((uint32_t) head + head->size - sizeof(footer_t));
            foot->magic = HEAP_MAGIC;
            foot->head = head;
        } else {
            uint32_t iter = 0;
            while ((iter < heap->index.size) && (lookup_oarr(iter, &heap->index) != (void *) testh)) {
                iter++;
            }

            if (iter < heap->index.size) {
                rm_oarr(iter, &heap->index);    
            }
        }
    }

    if (do_add == 1) {
        ins_oarr((void *) head, &heap->index);
    }
}

void kfree(void *p) {
    free(p, kheap);
}