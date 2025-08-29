#pragma once

#include <asm/io.h>
#include <int/task.h>

extern uint8_t is_tasking_enabled;

#define assert(c) __assert(c, __FILE__, __LINE__)

inline void __assert(int condition, char *file, uint32_t line) {
    if (!condition) {
        if (is_tasking_enabled && getpid() != -1) {
            int pid = getpid();
            serial_printf("Assertion failed for task %d in %s:%d\n", pid, file, line);

            kill_task(pid, SIGAFAIL);
        } else {
            serial_printf("Assertion failed in %s:%d\n", file, line);
            while (1) {
                asm("hlt");
            }
        }
    }
}