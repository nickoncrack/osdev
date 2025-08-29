#pragma once

#include <mm/kheap.h>
#include <mm/paging.h>

#define KERNEL_STACK_SIZE 4096
#define MAX_TASKS 64

#define SIGKILL 1 // kill
#define SIGSEGV 2 // segmentation fault
#define SIGAFAIL 3 // assertion failed

typedef enum {
    TASK_READY,
    TASK_RUNNING,
    TASK_BLOCKED,
    TASK_TERMINATED
} task_state_t;

struct process_address_space { // ONLY FOR USER TASKS
    uint16_t address_idx;
    uint32_t binary_size;

    /*
        buffer is placed at the end of the heap, leading to a smaller heap size

        bit 0 - is allowed to have a buffer?
        bit 1 - has requested a buffer?
    */
    uint8_t has_buffer : 2;
    uint32_t buffer_start;
} __attribute__((packed));

typedef struct {
    uint8_t user; // 1 for user task, 0 for kernel task
    struct process_address_space *addr;

    uint32_t pid;
    uint32_t sleep_expiry;
    uint32_t preempt_count;

    regs_t regs;
    heap_t *heap;
    pagedir_t *page_dir;

    uint16_t buf_w, buf_h;

    task_state_t state;
    uint8_t ret;
    uint8_t kernel_stack[KERNEL_STACK_SIZE];
} tcb_t; // task control block

void init_tasking();
int create_task(uint32_t eip);
int create_user_task(struct process_address_space *addr);
int scheduler_pick_next();

int getpid();
tcb_t *get_task(int pid);

void kill_task(int pid, int reason);

void preempt_disable();
void preempt_enable();

void do_schedule(regs_t *regs);