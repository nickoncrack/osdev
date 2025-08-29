#include <asm/io.h>
#include <int/isr.h>

#define INTERRUPT_NUM 256
isr_t interrupt_handlers[INTERRUPT_NUM];

void isr_handler(regs_t *regs) {
    if (interrupt_handlers[regs->int_no] != 0) {
        isr_t handler = interrupt_handlers[regs->int_no];
        handler(regs);
    } else {
        // unhandled interrupt
    }
}

void irq_ack(uint8_t int_no) {
    if (int_no >= 12) {
        outb(PIC_SLAVE_CMD, PIC_EOI);
    }
    outb(PIC_MASTER_CMD, PIC_EOI);
}

void irq_handler(regs_t *regs) {
    if (interrupt_handlers[regs->int_no] != 0) {
        isr_t handler = interrupt_handlers[regs->int_no];
        handler(regs);
    }

    irq_ack(regs->int_no);
}

void register_interrupt_handler(uint8_t n, isr_t handler) {
    // n <= 256
    interrupt_handlers[n] = handler;

    #ifdef DEBUG
    serial_printf("Registered interrupt handler for interrupt %d at 0x%x\n", n, (uint32_t)handler);
    #endif
}
