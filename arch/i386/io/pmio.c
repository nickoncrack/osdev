#include <common.h>

void outb(uint16_t port, uint8_t val) {
    asm volatile("outb %1, %0" : : "dN" (port), "a" (val));
}

void outw(uint16_t port, uint16_t val) {
    asm volatile("outw %1, %0" : : "dN" (port), "a"(val));
}

uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a" (ret) : "dN" (port));
    return ret;
}

uint16_t inw(uint16_t port) {
    uint16_t ret;
    asm volatile("inw %1, %0" : "=a" (ret) : "dN" (port));
    return ret;
}

uint32_t inl(uint16_t port) {
    uint32_t ret;
    asm volatile("inl %1, %0" : "=a" (ret) : "dN" (port));
    return ret;
}

void insl(uint16_t port, uint32_t *buffer, int quads) {
    for (int i = 0; i < quads; i++) {
        buffer[i] = inl(port);
    }
}