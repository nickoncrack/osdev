#define SSFN_CONSOLEBITMAP_TRUECOLOR
#include <ssfn.h>
#include <multiboot.h>

#include <fs/vfs.h>
#include <fs/skbdfs.h>

#include <bin.h>
#include <hw/ata.h>
#include <hw/keybaord.h>
#include <asm/io.h>
#include <int/idt.h>
#include <int/gdt.h>
#include <int/task.h>
#include <int/syscall.h>
#include <hal/acpi.h>
#include <mm/kheap.h>
#include <mm/paging.h>
#include <int/timer.h>
#include <video/vga.h>
#include <video/vbe.h>
#include <gui/compositor.h>

extern uint8_t _binary_console_sfn_start;
extern struct FADT *fadt;

extern tcb_t tasks[MAX_TASKS];
extern int ntasks;

uint32_t initial_esp;
struct mboot_info *mboot;

uint64_t fs_size = 0;

struct vbe_mode_info *vbe_info;

static void ssfn_cputs(char *s, uint32_t fg) {
    ssfn_dst.fg = fg;
    vesa_puts(s);
    ssfn_dst.fg = 0xFFFFFFFF;
}

static void isr13_handler(regs_t *r) {
    uint32_t *p = (uint32_t *) r;
    serial_printf("---General protection fault---\n");
    serial_printf("regs ptr = 0x%x\n", r);
    serial_printf("interpreted: eip=0x%x cs=0x%x eflags=0x%x esp=0x%x ss=0x%x int_no=13 err=0x%x\n",
                  r->eip, (uint16_t)r->cs, r->eflags, r->esp, (uint16_t)r->ss, r->err_code);

    for(;;) asm volatile("hlt");
}

static void isr06_handler(regs_t *r) {
    serial_puts("---Invalid opcode exception---\n");
    serial_printf("regs ptr = 0x%x\n", r);
    serial_printf("eip=0x%x cs=0x%x eflags=0x%x esp=0x%x ss=0x%x int_no=6 err=0\n",
                  r->eip, (uint16_t)r->cs, r->eflags, r->esp, (uint16_t)r->ss);

    for(;;) asm volatile("hlt");
}

extern void compositor_init();
void init() {
    vbe_info = (struct vbe_mode_info*) mboot->vbe_mode_info;
    init_vbe(vbe_info);
    #ifdef DEBUG
    serial_printf("Framebuffer initialized at 0x%x\n", LFB_VADDR);
    #endif

    ssfn_src = (ssfn_font_t *) &_binary_console_sfn_start;
    ssfn_dst.ptr = (uint8_t*) LFB_VADDR;
    ssfn_dst.p = vbe_info->pitch; // bytes per line
    ssfn_dst.fg = 0xFFFFFFFF; // white on black
    ssfn_dst.bg = 0x00000000;
    ssfn_dst.x = 1;
    ssfn_dst.y = 0;

    ssfn_putc('[');
    ssfn_cputs("ok", 0xFF00FF00); // green
    vesa_puts("] SSFN has been configured.\n");

    #ifdef DEBUG
    serial_puts("SSFN has been configured\n");
    #endif

    init_acpi();
    ssfn_putc('[');
    ssfn_cputs("ok", 0xFF00FF00);
    vesa_puts("] ACPI initialization completed.\n");

    #ifdef DEBUG
    serial_puts("ACPI initialization completed\n");
    #endif

    keyboard_init();

    vesa_puts("\nWelcome to ");
    ssfn_cputs("Radiant OS", 0xFF00FFFF);
    vesa_puts("!\n");

    // compositor_init();
    serial_puts("Late init complete\n");

    return;
}

extern _Bool is_cpuid_supported();
void kernel_init(struct mboot_info *mboot_ptr, uint32_t initial_stack) {
    initial_esp = initial_stack;
    mboot = mboot_ptr;

    init_gdt();
    init_idt();

    init_serial();

    register_interrupt_handler(0x06, isr06_handler);
    register_interrupt_handler(0x0D, isr13_handler);
    register_interrupt_handler(0x7F, syscall_handler);

    _Bool b = is_cpuid_supported();
    serial_printf("cpuid support check = %d\n", b);

    init_paging();

    pit_install(1000);

    // enable interrupts temporarily to calibrate the rdtsc
    asm volatile("sti");

    // calibrate rdtsc, (my host doesnt support cpuid 0x16)
    uint64_t start = rdtsc();
    ksleep(100);
    uint64_t end = rdtsc();

    uint64_t delta = end - start;
    uint64_t freq = (delta * 1000) / 100;

    asm volatile("cli");

    serial_printf("cpu freq ~ %lu Hz\n", freq);

    init_tasking();
    serial_puts("Early init complete\n");

    init_vfs();
    fs_size = get_drive_size(0);
    serial_printf("Device detected: id: 0, size: %u MiB\n", fs_size / 1024);

    init();

    create_task((uint32_t) compositor);

    struct process_address_space *addr = load("/bin/gui", 1);
    create_user_task(addr);

    asm volatile("sti");
    return;
}


