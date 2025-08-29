#include <asm/io.h>
#include <int/isr.h>
#include <hw/mouse.h>

int8_t mouse_data[4];
uint8_t i = 0;

_Bool btn5_support = 0;

uint8_t packet_length = 3;
mouse_packet_t mouse_packet;

static uint8_t mouse_read() {
    uint32_t timeout = 10000;
    while (timeout--) {
        if (inb(PS2_STATUS) & 1) break;
    }
    if (timeout == 0) {
        serial_printf("mouse read timeout\n");
        return 0;
    }

    uint8_t c = inb(PS2_DAT);
    return c;
}

static void mouse_write(uint8_t c) {
    while (inb(PS2_STATUS) & 2);
    outb(PS2_STATUS, PS2_WRITE_PORT2);

    while (inb(PS2_STATUS) & 2);
    outb(PS2_DAT, c);
}

void mouse_handler(regs_t *regs) {
    uint8_t status = inb(PS2_STATUS);
    while (status & 1) {
        char in = inb(PS2_DAT);

        if (i == 0) {
            // check bit 3 (always 1), bit 6,7 (overflow)
            if (!(in & PS2_MOUSE_BIT_A1) || \
                (in & PS2_MOUSE_BIT_XO) || \
                (in & PS2_MOUSE_BIT_YO)) {
                    irq_ack(12);
                    return;
                }
        }

        mouse_data[i] = in;

        if (i < packet_length) {
            i++;
            continue;
        }

        // i = 3 / i = 2
        mouse_packet.button = 0;
        mouse_packet.dx = mouse_data[1];
        mouse_packet.dy = mouse_data[2];
        
        if (!btn5_support && packet_length == 4) {
            mouse_packet.dz = mouse_data[3];   
        }

        if (mouse_data[0] & PS2_MOUSE_BIT_LMB) {
            mouse_packet.button |= LEFT_CLICK;
        }
        if (mouse_data[0] & PS2_MOUSE_BIT_RMB) {
            mouse_packet.button |= RIGHT_CLICK;
        }
        if (mouse_data[0] & PS2_MOUSE_BIT_MMB) {
            mouse_packet.button |= MIDDLE_CLICK;
        }

        if (btn5_support) {
            // bits 0-3 contain z axis data
            mouse_packet.dz = (mouse_data[3] & 0x0F);

            if (mouse_data[3] & PS2_MOUSE_BIT_BTN4) {
                mouse_packet.button |= BUTTON_4;
            }
            if (mouse_data[3] & PS2_MOUSE_BIT_BTN5) {
                mouse_packet.button |= BUTTON_5;
            }
        }

        i = 0;
    }

    irq_ack(12);
}

void set_mouse_rate(uint8_t rate) {
    mouse_write(PS2_MOUSE_SET_RATE);
    mouse_read();
    mouse_write(rate);
    mouse_read();
}

void mouse_init() {
    uint8_t status;

    // disable ports
    outb(PS2_STATUS, PS2_DIS_PORT1);
    outb(PS2_STATUS, PS2_DIS_PORT2);

    while (inb(PS2_STATUS) & 1) {
        inb(PS2_DAT); // flush buffer
    }

    // read config byte
    outb(PS2_STATUS, PS2_READ0);
    while (!(inb(PS2_STATUS) & 1));
    status = inb(PS2_DAT);
    status |= 1; // unmask irq01
    status |= 2; // unmask irq12
    status &= ~(1 << 6); // disable translation

    // write config byte
    outb(PS2_STATUS, PS2_WRITE0);
    while (inb(PS2_STATUS) & 2);
    outb(PS2_DAT, status);

    // enable ports
    outb(PS2_STATUS, PS2_ENB_PORT1);
    outb(PS2_STATUS, PS2_ENB_PORT2);

    // test ps/2 controller
    outb(PS2_STATUS, PS2_TEST);
    while (!(inb(PS2_STATUS) & 1));
    uint8_t test = inb(PS2_DAT);
    if (test != PS2_TEST_CTRL_PASS) {
        serial_printf("PS/2 controller test failed\n");
        return;
    }

    // test ps/2 port 1
    outb(PS2_STATUS, PS2_TEST_PORT1);
    while (!(inb(PS2_STATUS) & 1));
    test = inb(PS2_DAT);
    if (test != PS2_TEST_PORT_PASS) {
        serial_printf("PS/2 port 1 test failed\n");
        return;
    }

    // test ps/2 port 2
    outb(PS2_STATUS, PS2_TEST_PORT2);
    while (!(inb(PS2_STATUS) & 1));
    test = inb(PS2_DAT);
    if (test != PS2_TEST_PORT_PASS) {
        serial_printf("PS/2 port 2 test failed\n");
        return;
    }

    // reset
    mouse_write(PS2_MOUSE_RESET);
    uint8_t ack = mouse_read();
    if (ack != PS2_ACK) {
        serial_printf("Mouse test failed\n");
        return;
    }

    uint8_t bat = mouse_read();
    uint8_t id = mouse_read();

    serial_printf("mouse: bat = 0x%x, id = 0x%x\n", bat, id);

    // set defaults
    mouse_write(PS2_MOUSE_DEFAULT);
    mouse_read();

    mouse_write(PS2_DIS_SCAN);
    mouse_read();

    // enable z axis
    set_mouse_rate(200);
    set_mouse_rate(100);
    set_mouse_rate(80);

    // identify
    mouse_write(PS2_IDENTIFY);
    mouse_read();
    id = mouse_read();

    // enable 5 buttons
    if (id == PS2_ID_SCRL_MOUSE) {
        packet_length = 4;

        set_mouse_rate(200);
        set_mouse_rate(200);
        set_mouse_rate(80);

        mouse_write(PS2_IDENTIFY);
        mouse_read();
        id = mouse_read();

        if (id == PS2_ID_5BTN_MOUSE) {
            btn5_support = 1;
        }
    }

    // enable streaming
    mouse_write(PS2_ENB_SCAN);
    mouse_read();

    register_interrupt_handler(IRQ(12), mouse_handler);
}
