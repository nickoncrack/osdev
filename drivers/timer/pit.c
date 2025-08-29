#include <asm/io.h>

#include <int/gdt.h>
#include <int/isr.h>
#include <int/timer.h>
#include <int/task.h>

extern tcb_t tasks[MAX_TASKS];
extern int ntasks;
extern int crt_task;
extern uint8_t is_tasking_enabled;

// max uptime ~49 days
volatile uint32_t tick = 0;

static void pit_callback(regs_t *regs) {
    tick++;
    irq_ack(0);

    if (!is_tasking_enabled) return;

    do_schedule(regs);

    // irq stub will now iret into the contents of regs
}

void ksleep(uint32_t ms) {
    if (crt_task == -1) {
        uint32_t start = tick;
        while (pit_get_ticks() < start + ms) asm("hlt");

        return;
    }

    asm volatile("cli");

    tasks[crt_task].state = TASK_BLOCKED;
    tasks[crt_task].sleep_expiry = ms;

    #ifdef DEBUG
    serial_printf("sleep: crt_task = %d is yielding\n", crt_task);
    #endif

    // yield
    asm volatile("int $0x20"); // ensure atomic
    asm volatile("sti");

    return;
}

void pit_install(uint32_t freq) {
    register_interrupt_handler(IRQ(0), pit_callback);

    uint32_t div = 1193182 / freq;
    outb(PIT_CMD, PIT_SET);

    uint8_t low = div & 0xFF;
    uint8_t high = (div >> 8) & 0xFF;

    outb(PIT_DAT0, low);
    outb(PIT_DAT0, high);
}

uint32_t pit_get_ticks() {
    return tick;
}