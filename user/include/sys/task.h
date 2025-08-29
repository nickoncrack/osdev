#pragma once

#include <sys/exec.h>

typedef uint32_t pid_t;

typedef enum {
    TASK_READY,
    TASK_RUNNING,
    TASK_BLOCKED,
    TASK_TERMINATED
} task_state_t;

pid_t create_task(struct process_address_space *addr);
int wait(pid_t pid);