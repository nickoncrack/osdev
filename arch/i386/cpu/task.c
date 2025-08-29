#include <bin.h>
#include <string.h>

#include <asm/io.h>
#include <int/gdt.h>
#include <int/task.h>
#include <mm/paging.h>

uint8_t is_tasking_enabled = 0;

tcb_t tasks[MAX_TASKS];
volatile int crt_task = -1;
int ntasks = 0;

void check_stack_usage(tcb_t *task) {
    if (task->regs.esp < (uint32_t)task->kernel_stack ||
        task->regs.esp >= (uint32_t)&task->kernel_stack[KERNEL_STACK_SIZE]) {
        serial_printf("Stack overflow detected for task %d esp=0x%x\n", task->pid, task->regs.esp);
        while (1) asm("hlt");
    }
}

int getpid() {
    return crt_task;
}

tcb_t *get_task(int pid) {
    if (pid < 0 || pid >= ntasks) {
        return NULL;
    }

    return &tasks[pid];
}

void cleanup_terminated_tasks() {
    for (int i = 0; i < ntasks;) {
        if (tasks[i].state == TASK_TERMINATED) {
            serial_printf("Cleaning task %d (ret=%d)\n", tasks[i].pid, tasks[i].ret);

            // shift remaining tasks down
            for (int j = i; j < ntasks - 1; j++) {
                tasks[j] = tasks[j + 1];
            }
            ntasks--;
        } else {
            i++;
        }
    }
}

void idle_task() {
    serial_printf("idle_task(): Idle task started\n");
    while (1) {
        cleanup_terminated_tasks();
        asm volatile("sti; hlt");
    }
}

void init_tasking() {
    tcb_t *idle = &tasks[0];
    idle->pid = 0;

    idle->state = TASK_RUNNING;
    idle->sleep_expiry = 0;
    idle->ret = 0;
    idle->user = 0;

    // idle task (pid = 0)
    memset(&idle->regs, 0, sizeof(regs_t));
    idle->regs.cs = 0x08;
    idle->regs.ds = 0x10;
    idle->regs.ss = 0x10;
    idle->regs.eflags = 0x202;
    idle->regs.esp = (uint32_t) &idle->kernel_stack[KERNEL_STACK_SIZE - 1];
    idle->regs.ebp = idle->regs.esp;
    idle->regs.eip = (uint32_t) idle_task;

    idle->heap = NULL; // idle doesnt need a heap
    idle->page_dir = clone_dir(kernel_dir);
    map_memory(
        (uint32_t)idle->kernel_stack,
        (uint32_t)idle->kernel_stack,
        KERNEL_STACK_SIZE,
        idle->page_dir,
        1, 0
    );

    ntasks = 1;
    crt_task = -1;

    is_tasking_enabled = 1;

    switch_page_dir(kernel_dir);

    #ifdef DEBUG
    serial_printf("init_tasking(): Tasking initialized with idle task at 0x%x\n", (uint32_t)idle_task);
    #endif
}

void __task_exit(int ret) {
    tasks[crt_task].state = TASK_TERMINATED;
    tasks[crt_task].ret = ret;
    serial_printf("Task %d exited with code %d\n", crt_task, ret);
    asm volatile("int $0x20");

    for (;;) asm("hlt");
}


int create_user_task(struct process_address_space *addr) {
    tcb_t *task = &tasks[ntasks];
    task->user = 1;
    task->pid = ntasks;
    task->state = TASK_READY;
    task->sleep_expiry = 0;
    task->ret = 0;

    task->page_dir = clone_dir(kernel_dir);

    uint32_t entry = BIN_BASE_ADDR + addr->address_idx * MAX_PROCESS_SIZE; 
    uint32_t end_code = entry + addr->binary_size;
    uint32_t stack_top = entry + MAX_PROCESS_SIZE - 1;

    memset(&task->regs, 0, sizeof(regs_t));
    task->regs.ds = 0x23; // user ds
    task->regs.es = 0x23;
    task->regs.fs = 0x23;
    task->regs.gs = 0x23;
    task->regs.ss = 0x23;
    task->regs.cs = 0x1B; // user cs

    task->regs.eip = BIN_BASE_ADDR + addr->address_idx * MAX_PROCESS_SIZE; // binary entry point
    task->regs.esp = stack_top;
    task->regs.ebp = task->regs.esp;
    task->regs.useresp = stack_top;
    task->regs.eflags = 0x202; // interrupt flag enabled

    task->addr = addr;

    // allocate user stack
    uint32_t stack_bottom = stack_top - PROCESS_STACK_SIZE + 1; // 128 kb stack

    // creating a guard page will cause a page fault when a stack overflow happens
    uint32_t guard_page = stack_bottom - 0x1000;
    unmap_memory(guard_page, 0x1000, task->page_dir); // explicitly unmapped

    /*
        if the task has a buffer, it will be placed at the end of the heap region,
        however, the buffer size is determined at runtime by the user task,
        therefore, a task that will get a buffer, temporarily won't have a heap either.
        the heap will be created when the user task requests a buffer.

        this design is very limiting, as a task that needs a buffer cannot use the heap until it requests one.
        but I couldn't think of a simpler alternative that doesn't involve resizing the heap at runtime
        
        so the first thing a window task has to do is to request a buffer.
    */
    if (!addr->has_buffer) {
        // round heap start to the next page
        uint32_t heap_start = end_code + 0x1000 - (end_code % 0x1000);

        task->heap = mkheap(
            heap_start,
            heap_start + KHEAP_INITIAL_SZ,
            guard_page - 1,
            0, 0 // user, r/w
        );
    }

    serial_printf("create_task(): User task %d created: eip=0x%x esp=0x%x\n",
              task->pid, task->regs.eip, task->regs.esp);

    ntasks++;
    return task->pid;
}

int create_task(uint32_t eip) {
    if (ntasks >= MAX_TASKS) {
        return -1;
    }

    tcb_t *task = &tasks[ntasks];
    task->user = 0;
    task->pid = ntasks;
    task->state = TASK_READY;
    task->sleep_expiry = 0;
    task->ret = 0;

    memset(&task->regs, 0, sizeof(regs_t));
    task->regs.cs = 0x08; // kernel cs
    task->regs.ds = 0x10; // kernel ds
    task->regs.es = 0x10;
    task->regs.fs = 0x10;
    task->regs.gs = 0x10;
    task->regs.ss = 0x10; // kernel ss
    task->regs.eflags = 0x202; // int eflag enabled
    task->regs.eip = eip;
    task->regs.esp = (uint32_t) &task->kernel_stack[KERNEL_STACK_SIZE - 1];

    task->regs.ebp = task->regs.esp;

    task->page_dir = clone_dir(kernel_dir);

    serial_printf("create_task(): Task %d created: eip=0x%x esp=0x%x\n",
              task->pid, task->regs.eip, task->regs.esp);

    ntasks++;
    return task->pid;
}

int scheduler_pick_next() {
    if (ntasks == 0) return -1;

    int start = (crt_task + 1) % ntasks;
    for (int i = 0; i < ntasks; i++) {
        int idx = (start + i) % ntasks;
        if (tasks[idx].state == TASK_READY) {
            return idx;
        }
    }

    if (crt_task != -1 && tasks[crt_task].state == TASK_RUNNING) {
        return crt_task;
    }

    return -1;
}

void kill_task(int pid, int reason) {
    if (pid < 0 || pid >= ntasks) {
        serial_printf("kill_task(): Invalid PID %d\n", pid);
        return;
    }

    tcb_t *task = &tasks[pid];
    if (task->state != TASK_TERMINATED) {
        task->state = TASK_TERMINATED;
        task->ret = reason;
        serial_printf("kill_task(): Task %d terminated, reason = %d\n", pid, reason);
    } else {
        serial_printf("kill_task(): Task %d is already terminated\n", pid);
    }

    asm volatile("int $0x20");
}

// this function will never be called from kernel mode
void *malloc_int(uint32_t size) {
    if (size == 0 || crt_task == -1 || !tasks[crt_task].heap) {
        return NULL; // invalid size, tasking not enabled or current task is idle
    }

    return alloc(size, 0, tasks[crt_task].heap);
}

void free_int(void *ptr) {
    if (ptr == NULL || crt_task == -1 || !tasks[crt_task].heap) {
        return;
    }

    free(ptr, tasks[crt_task].heap);
}

void preempt_disable() {
    if (crt_task != -1) {
        tasks[crt_task].preempt_count++;
    }
}

void preempt_enable() {
    if (crt_task != -1) {
        tasks[crt_task].preempt_count--;
    }
}

void do_schedule(regs_t *regs) {
    asm volatile("cli");

    // save current interrupt frame to tcb
    if (crt_task != -1) {
        tasks[crt_task].regs = *regs;
    }

    // wake up sleeping tasks
    for (int i = 0; i < ntasks; i++) {
        if (tasks[i].state == TASK_BLOCKED && tasks[i].sleep_expiry > 0){
            tasks[i].sleep_expiry--;
            if (tasks[i].sleep_expiry == 0) {
                tasks[i].state = TASK_READY;
            }
        }
    }

    if (crt_task != -1 && tasks[crt_task].preempt_count > 0) {
        asm volatile("sti");
        return;
    }

    int next = scheduler_pick_next();

    // idle task
    if (next == -1) {
        next = 0;
    }

    if (next == crt_task) {
        asm volatile("sti");
        return;
    }

    // idle has minimum priority so it should never be set to ready.
    if (crt_task != -1 && tasks[crt_task].state == TASK_RUNNING && tasks[crt_task].pid != 0) {
        tasks[crt_task].state = TASK_READY;
    }

    tasks[next].state = TASK_RUNNING;
    crt_task = next;

    switch_page_dir(tasks[next].page_dir);
    set_kernel_stack((uint32_t) &tasks[next].kernel_stack[KERNEL_STACK_SIZE - 1]); // set tss.esp0

    // overwrite saved irq frame so iret will jump into the next task
    *regs = tasks[next].regs;

    if (tasks[crt_task].user) {
        regs->esp = regs->useresp;
    }
}
