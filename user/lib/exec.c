#include <stdio.h>
#include <stdlib.h>

#include <sys/task.h>
#include <sys/syscall.h>

static struct process_address_space *load(char *path) {
    struct process_address_space *addr = malloc(sizeof(struct process_address_space));
    if (!addr) {
        return NULL;
    }

    uint32_t ret = 0;

    asm volatile(
        "int $0x7F"
        : "=a"(ret)
        : "a"(SYS_LOAD), "b"((uint32_t) path), "c"((uint32_t) addr)
        : "memory"
    );

    asm volatile("" ::: "memory");

    if (ret != 0) {
        free(addr);
        return NULL;
    }

    return addr;
}

void destroy_process(struct process_address_space *s) {
    if (!s) return;

    asm volatile(
        "int $0x7F"
        :: "a"(SYS_DPROC), "b"((uint32_t) s)
        : "memory"
    );

    asm volatile("" ::: "memory");

    free(s);
    return;
}

int exec(char *path) {
    struct process_address_space *addr = load(path);
    if (!addr) {
        return -1; // error loading binary
    }

    pid_t pid = create_task(addr);

    int ret = wait(pid);

    destroy_process(addr);
    return ret;
}