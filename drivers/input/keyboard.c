#include <stdio.h>
#include <asm/io.h>
#include <int/isr.h>
#include <video/vbe.h>

const char sc_ascii_shift[] = {'?', '?', '!', '@', '#', '$', '%', '^',
                         '&', '*', '(', ')', '_', '+', '?', '?', 'Q', 'W', 'E', 'R', 'T', 'Y',
                         'U', 'I', 'O', 'P', '{', '}', '?', '?', 'A', 'S', 'D', 'F', 'G',
                         'H', 'J', 'K', 'L', ':', '\"', '~', '/', '|', 'Z', 'X', 'C', 'V',
                         'B', 'N', 'M', '<', '>', '?', '?', '?', '?', ' '};
const char sc_ascii[] = {'?', '?', '1', '2', '3', '4', '5', '6',
                         '7', '8', '9', '0', '-', '=', '?', '?', 'q', 'w', 'e', 'r', 't', 'y',
                         'u', 'i', 'o', 'p', '[', ']', '?', '?', 'a', 's', 'd', 'f', 'g',
                         'h', 'j', 'k', 'l', ';', '\'', '`', '?', '\\', 'z', 'x', 'c', 'v',
                         'b', 'n', 'm', ',', '.', '/', '?', '?', '?', ' '};

#define ENTER 0x1C
#define BACKSPACE 0x0E

#define SHIFT 0x2A
#define RSHIFT 0x36
#define SHIFT_RELEASE 0xAA
#define RSHIFT_RELEASE 0xB6

static uint8_t shift = 0;

static int idx = 0;
char buffer[256];

// this will be set to 1 when fread(stdin) is called
_Bool reading = 0;

// will be set to 1 when enter is pressed, and back to 0 when the buffer is read
_Bool finished_reading = 0;

static void keyboard_callback(regs_t *regs) {
    uint8_t status;
    uint16_t scancode;

    status = inb(PS2_STATUS);
    if (status & 0x01) {
        if (!reading) {
            // if we are not reading, ignore the scancode
            outb(PIC_MASTER_CMD, PIC_EOI);
            return;
        }

        scancode = inb(PS2_STATUS);

        if (scancode == BACKSPACE) {
            idx--;
            if (buffer >= 0) buffer[idx] = 0; // remove last character
            vesa_putc('\b');
        } else if (scancode == SHIFT || scancode == RSHIFT) {
            shift = 1;
        } else if (scancode == SHIFT_RELEASE || scancode == RSHIFT_RELEASE) {
            shift = 0;
        } else if (scancode == ENTER) {
            reading = 0; // stop reading
            finished_reading = 1; // signal that we finished reading
            vesa_putc('\n');
        }

        if (scancode <= 57 && sc_ascii[scancode] != '?') {
            char c = (shift) ? sc_ascii_shift[scancode] : sc_ascii[scancode];
            buffer[idx++] = c;
            vesa_putc(c);
            inb(PS2_STATUS);
        }
    }

    outb(PIC_MASTER_CMD, PIC_EOI);
}

void read_buffer(file_t *unused, uint32_t size, uint8_t *dst) {
    if (reading) return;

    // interrupts are disabled by the stub
    asm volatile("sti");

    dst[0] = 0; // clear the buffer

    reading = 1;
    finished_reading = 0;

    while (!finished_reading) asm("hlt");

    reading = 0;
    for (int i = 0; i < idx && i < size; i++) {
        dst[i] = buffer[i];
    }

    dst[idx] = 0; // null-terminate the string
    idx = 0; // reset the index for the next read
    finished_reading = 0; // reset the finished reading flag

    for (int i = 0; i < 256; i++) {
        buffer[i] = 0; // clear the buffer
    }
}

void keyboard_init() {
    register_interrupt_handler(IRQ(1), keyboard_callback);
}