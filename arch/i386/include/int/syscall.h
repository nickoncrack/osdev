#pragma once

#include <int/isr.h>

typedef uint32_t (*syscall_t)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);

uint32_t sys_exit(uint32_t ret, uint32_t unused1, uint32_t unused2, uint32_t unused3, uint32_t unused4);
uint32_t sys_write(uint32_t fd, uint32_t size, uint32_t buf, uint32_t unused1, uint32_t unused2);
uint32_t sys_read(uint32_t fd, uint32_t size, uint32_t buf, uint32_t unused1, uint32_t unused2);
uint32_t sys_open(uint32_t path, uint32_t mode, uint32_t unused1, uint32_t unused2, uint32_t unused3);
uint32_t sys_malloc(uint32_t size, uint32_t unused1, uint32_t unused2, uint32_t unused3, uint32_t unused4);
uint32_t sys_free(uint32_t addr, uint32_t unused1, uint32_t unused2, uint32_t unused3, uint32_t unused4);
uint32_t sys_listdir(uint32_t user_path, uint32_t user_buffer, uint32_t buffer_size, uint32_t node_count_ptr, uint32_t unused1);
uint32_t sys_load(uint32_t user_path, uint32_t dst, uint32_t unused1, uint32_t unused2, uint32_t unused3);
uint32_t sys_destroy_process(uint32_t pad, uint32_t unused1, uint32_t unused2, uint32_t unused3, uint32_t unused4);
uint32_t sys_create_task(uint32_t addr, uint32_t unused1, uint32_t unused2, uint32_t unused3, uint32_t unused4);
uint32_t sys_get_state(uint32_t pid, uint32_t ptr, uint32_t unused1, uint32_t unused2, uint32_t unused3);
uint32_t sys_get_display_info(uint32_t vbe_info_ptr, uint32_t unused1, uint32_t unused2, uint32_t unused3, uint32_t unused4);
uint32_t sys_request_buffer(uint32_t width, uint32_t height, uint32_t unused1, uint32_t unused2, uint32_t unused3);
uint32_t sys_buffer_ready(uint32_t x, uint32_t y, uint32_t unused1, uint32_t unused2, uint32_t unused3);

extern uint32_t NUM_SYSCALLS;

void syscall_handler(regs_t *regs);

int copy_from_user(void *dst, void *user_src, uint32_t size);
int copy_to_user(void *user_dst, void *src, uint32_t size);