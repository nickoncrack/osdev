#include <stdio.h>
#include <sys/task.h>
#include <sys/syscall.h>

pid_t create_task(struct process_address_space *addr) {
    pid_t pid = -1;

    asm volatile(
        "int $0x7F"
        : "=a"(pid)
        : "a"(SYS_NEWTASK), "b"((uint32_t) addr)
        : "memory"
    );

    return pid;
}

int wait(pid_t pid) {
    int ret = -1;
    task_state_t state = TASK_READY;

    while (state != TASK_TERMINATED) {
        asm volatile(
            "int $0x7F"
            : "=a"(state)
            : "a"(SYS_GETSTATE), "b"(pid), "c"((uint32_t) &ret)
            : "memory"
        );
        asm volatile("" ::: "memory");
    }

    return ret;
}