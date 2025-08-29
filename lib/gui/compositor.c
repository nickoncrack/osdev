#include <string.h>

#include <asm/io.h>
#include <int/task.h>
#include <int/timer.h>
#include <video/vbe.h>
#include <gui/compositor.h>

buffer_queue_t *queue;

void compositor_init() {
    queue = (buffer_queue_t *) kmalloc(sizeof(buffer_queue_t));

    queue->pid = -1;
    queue->x = queue->y = 0;
    queue->next = NULL;

    return;
}

void add_buffer_to_queue(uint32_t pid, uint16_t x, uint16_t y) {
    if (queue->pid == -1) {
        // queue is empty
        queue->pid = pid;
        queue->x = x;
        queue->y = y;

        return;
    }

    buffer_queue_t *temp = queue;

    // last element is allocated with next = null
    while (temp->next != NULL) temp = temp->next;

    buffer_queue_t *last = (buffer_queue_t *) kmalloc(sizeof(buffer_queue_t));
    last->pid = pid;
    last->x = x;
    last->y = y;
    last->next = NULL;

    temp->next = last;

    return;
}

extern uint32_t width;
static void draw(uint32_t addr, uint32_t pid, uint16_t x, uint16_t y) {
    if (x >= SCREEN_WIDTH || y >= SCREEN_HEIGHT) return;

    switch_page_dir(kernel_dir);

    // draw row by row
    uint16_t buf_w = get_task(pid)->buf_w;
    uint16_t buf_h = get_task(pid)->buf_h;

    uint16_t w = buf_w;
    uint16_t h = buf_h;

    // check for clipping
    if (x + buf_w > SCREEN_WIDTH) {
        w = buf_w - (SCREEN_WIDTH - x);
    }
    if (y + buf_h > SCREEN_HEIGHT) {
        h = buf_h - (SCREEN_HEIGHT - y);
    }

    uint32_t *dst_base = (uint32_t *) LFB_VADDR;
    uint32_t *src_base = (uint32_t *) addr;

    for (int row = 0; row < h; row++) {
        uint32_t *dst = dst_base + (y + row) * width;
        uint32_t *src = src_base + row * w;
        
        memcpy(dst, src, w * 4);
    }
}

void draw_buffers() {
    if (queue->pid == -1) return;

    preempt_disable();

    while (queue != NULL) {
        if (queue->pid != -1) {
            uint32_t buf = BUFFER(queue->pid);
            draw(buf, queue->pid, queue->x, queue->y);
        }

        buffer_queue_t *next = queue->next;
        kfree(queue);
        queue = next;
    }

    // queue is empty
    queue = (buffer_queue_t *) kmalloc(sizeof(buffer_queue_t));
    queue->pid = -1;
    queue->x = queue->y = 0;
    queue->next = NULL;

    preempt_enable();

    return;
}

void wait_for_vsync() {
    while (inb(0x3DA) & 0x08); // wait until retrace ends
    while (!(inb(0x3DA) & 0x08)); // wait until retrace starts
}

void compositor() {
    while (1) {
        draw_buffers();

        uint32_t start = pit_get_ticks();
        while (pit_get_ticks() < start + 16) {
            asm volatile("hlt");
        }
    }
}