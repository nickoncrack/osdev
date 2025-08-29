#pragma once

#include <common.h>


#define PIC_MASTER_CMD 0x20
#define PIC_MASTER_DAT 0x21
#define PIC_SLAVE_CMD  0xA0
#define PIC_SLAVE_DAT  0xA1

#define PIC_EOI 0x20

#define VGA_CMD 0x3D4
#define VGA_DAT 0x3D5

#define PS2_READ0       0x20
#define PS2_WRITE0      0x60
#define PS2_DAT         0x60
#define PS2_STATUS      0x64
#define PS2_WRITE_PORT2 0xD4
#define PS2_DIS_PORT2   0xA7
#define PS2_ENB_PORT2   0xA8
#define PS2_TEST_PORT2  0xA9
#define PS2_TEST        0xAA
#define PS2_TEST_PORT1  0xAB
#define PS2_DIS_PORT1   0xAD
#define PS2_ENB_PORT1   0xAE
#define PS2_IDENTIFY    0xF2
#define PS2_ENB_SCAN    0xF4
#define PS2_DIS_SCAN    0xF5
#define PS2_ACK         0xFA

#define PS2_ID_STD_MOUSE    0x00
#define PS2_ID_SCRL_MOUSE   0x03
#define PS2_ID_5BTN_MOUSE   0x04

#define PS2_TEST_CTRL_PASS  0x55
#define PS2_TEST_CTRL_FAIL  0xFC
#define PS2_TEST_PORT_PASS  0x00
#define PS2_TEST_CLK_LO     0x01
#define PS2_TEST_CLK_HI     0x02
#define PS2_TEST_DAT_LO     0x03
#define PS2_TEST_DAT_HI     0x04

#define PS2_MOUSE_SET_RATE  0xF3
#define PS2_MOUSE_DEFAULT   0xF6
#define PS2_MOUSE_RESET     0xFF
#define PS2_MOUSE_BIT_LMB   0x01
#define PS2_MOUSE_BIT_RMB   0x02
#define PS2_MOUSE_BIT_MMB   0x04
#define PS2_MOUSE_BIT_A1    0x08
#define PS2_MOUSE_BIT_BTN4  0x10
#define PS2_MOUSE_BIT_BTN5  0x20
#define PS2_MOUSE_BIT_XS    0x10
#define PS2_MOUSE_BIT_YS    0x20
#define PS2_MOUSE_BIT_XO    0x40
#define PS2_MOUSE_BIT_YO    0x80

#define PIT_SET  0x36
#define PIT_DAT0 0x40
#define PIT_DAT1 0x41
#define PIT_DAT2 0x42
#define PIT_CMD  0x43

#define CMOS_CMD 0x70
#define CMOS_DAT 0x71

#define COM1 0x3F8

uint8_t inb(uint16_t port);
uint16_t inw(uint16_t port);
uint32_t inl(uint16_t port);
void insl(uint16_t port, uint32_t *buffer, int quads);
void outb(uint16_t port, uint8_t val);
void outw(uint16_t port, uint16_t val);

int init_serial();
void serial_putc(char c);
void serial_puts(char *s);
void serial_printf(const char *s, ...);

uint32_t read_eflags();
void write_eflags(uint32_t eflags);
