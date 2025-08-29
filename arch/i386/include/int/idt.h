#pragma once

#include <common.h>

typedef struct {
    uint16_t base_lo;
    uint16_t sel;
    uint8_t reserved;
    uint8_t flags;
    uint16_t base_hi;
} PACKED idt_entry_t;

typedef struct {
    uint16_t lim;
    uint32_t base;
} PACKED idt_ptr_t;

#define DECL_ISR(n) extern void isr##n()
#define DECL_IRQ(n) extern void irq##n()

DECL_ISR(0);
DECL_ISR(1);
DECL_ISR(2);
DECL_ISR(3);
DECL_ISR(4);
DECL_ISR(5);
DECL_ISR(6);
DECL_ISR(7);
DECL_ISR(8);
DECL_ISR(9);
DECL_ISR(10);
DECL_ISR(11);
DECL_ISR(12);
DECL_ISR(13);
DECL_ISR(14);
DECL_ISR(15);
DECL_ISR(16);
DECL_ISR(17);
DECL_ISR(18);
DECL_ISR(19);
DECL_ISR(20);
DECL_ISR(21);
DECL_ISR(22);
DECL_ISR(23);
DECL_ISR(24);
DECL_ISR(25);
DECL_ISR(26);
DECL_ISR(27);
DECL_ISR(28);
DECL_ISR(29);
DECL_ISR(30);
DECL_ISR(31);
DECL_ISR(255);

DECL_IRQ(0);
DECL_IRQ(1);
DECL_IRQ(2);
DECL_IRQ(3);
DECL_IRQ(4);
DECL_IRQ(5);
DECL_IRQ(6);
DECL_IRQ(7);
DECL_IRQ(8);
DECL_IRQ(9);
DECL_IRQ(10);
DECL_IRQ(11);
DECL_IRQ(12);
DECL_IRQ(13);
DECL_IRQ(14);
DECL_IRQ(15);

// syscall
DECL_IRQ(127);

#undef DECL_ISR
#undef DECL_IRQ

void init_idt();