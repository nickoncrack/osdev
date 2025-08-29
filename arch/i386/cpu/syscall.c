#include <int/syscall.h>
#include <int/task.h>
#include <asm/io.h>

#include <bin.h>
#include <stdint.h>
#include <video/vbe.h>
#include <fs/vfs.h>
#include <mm/kheap.h>
#include <gui/compositor.h>

#include <assert.h>
#include <errno.h>
#include <string.h>


extern void __task_exit(int);
extern void *malloc_int(uint32_t);
extern void free_int(void*);

extern _Bool addresses[MAX_PROCESS];

syscall_t syscall_table[] = {
    sys_exit,
    sys_write,
    sys_read,
    sys_open,
    sys_malloc,
    sys_free,
    sys_listdir,
    sys_load,
    sys_destroy_process,
    sys_create_task,
    sys_get_state,
    sys_get_display_info,
    sys_request_buffer,
    sys_buffer_ready
};

uint32_t NUM_SYSCALLS = sizeof(syscall_table) / sizeof(syscall_t);

// user tasks (binaries most of the time) live in 0x40000000 - 0x80000000
static _Bool is_user_address(const void *ptr) {
    uint32_t addr = (uint32_t) ptr - BIN_BASE_ADDR;
    uint32_t user_region_size = BIN_END_ADDR - BIN_BASE_ADDR;

    // address not in user region or not allocated
    if (addr > user_region_size || addresses[addr / MAX_PROCESS_SIZE] == 0) {
        return 0;
    }

    return 1;
}

int copy_from_user(void *dst, void *user_src, uint32_t size) {
    if (!is_user_address(user_src)) {
        return EFAULT;
    }

    memcpy(dst, user_src, size);
    return 0;
}

int copy_to_user(void *user_dst, void *src, uint32_t size) {
    if (!is_user_address(user_dst)) {
        return EFAULT;
    }

    memcpy(user_dst, src, size);
    return 0;
}

uint32_t sys_exit(uint32_t ret, uint32_t unused1, uint32_t unused2, uint32_t unused3, uint32_t unused4) {
    __task_exit(ret);
    
    __builtin_unreachable();
}

// fd -> device index/file_t pointer
uint32_t sys_write(uint32_t fd, uint32_t size, uint32_t buf, uint32_t unused1, uint32_t unused2) {
    file_t *dev = dev_by_idx(fd);
    if (dev != NULL) {
        if ((dev->mode & MODE_W) != 0) {
            return dev->device->write(dev, size, (uint8_t *) buf);
        }
        return EPERM;
    } else {
        // if fd is not a device, it must be a file_t pointer
        file_t *file = (file_t *) fd;
        if (file->type == FILE_NORMAL && (file->mode & FMODE_W) != 0) {
            return vfs_write(file, size, (uint8_t *) buf);
        }
        return EPERM;
    }

    __builtin_unreachable();
}

uint32_t sys_read(uint32_t fd, uint32_t size, uint32_t buf, uint32_t unused1, uint32_t unused2) {
    file_t *dev = dev_by_idx(fd);
    if (dev != NULL) {
        if ((dev->mode & MODE_R) != 0) {
            return dev->device->read(dev, size, (uint8_t *) buf);
        }
        return EPERM;
    }

    return ENODEV;
}

// only for normal files, devices use indexes
uint32_t sys_open(uint32_t path, uint32_t mode, uint32_t unused1, uint32_t unused2, uint32_t unused3) {
    char *cpath = (char *) path;
    file_t *file = vfs_open(cpath, mode);
    
    if (file == NULL) return ENOENT;

    return (uint32_t) file;
}

uint32_t sys_malloc(uint32_t size, uint32_t unused1, uint32_t unused2, uint32_t unused3, uint32_t unused4) {
    void *ptr = malloc_int(size);

    if (ptr == NULL) return (uint32_t) -1;

    return (uint32_t) ptr;
}

uint32_t sys_free(uint32_t addr, uint32_t unused1, uint32_t unused2, uint32_t unused3, uint32_t unused4) {
    if (addr) {
        free_int((void *) addr);
        return 0;
    }
    return (uint32_t) -1;
}

/* 
    this function will copy kernel heap data to user buffers so the data can be freed from a user task.

    How it works:
    This function will be called twice.
    1. First it will be called with user_buffer = buffer_size = 0 in order to retrieve the number of nodes in the directory.
    2. Then it will be called to retrieve the actual data
*/
uint32_t sys_listdir(uint32_t user_path, uint32_t user_buffer, uint32_t buffer_size, uint32_t node_count_ptr, uint32_t unused1) {
    char kernel_path[256] = {0};

    // copy user input to kernel space
    if (copy_from_user(kernel_path, (void *) user_path, sizeof(kernel_path))) {
        return EFAULT;
    }

    node_t node = {0};
    find_node(kernel_path, FS_DIR, 0, &node);

    assert(node.magic == NODE_MAGIC);
    uint32_t count = node.size;

    if (node_count_ptr) {
        if (copy_to_user((void *) node_count_ptr, &count, sizeof(uint32_t))) {
            return EFAULT;
        }
        return 0; // only count requested
    }

    node_t **nodes = listdir(kernel_path);

    uint32_t needed_size = sizeof(node_t) * count;

    if (buffer_size < needed_size) {
        for (uint32_t i = 0; i < count; i++) {
            kfree(nodes[i]);
        }
        kfree(nodes);
        return ENOMEM;
    }

    node_t *nodes_buffer = (node_t *) kmalloc(needed_size);
    if (!nodes_buffer) {
        for (uint32_t i = 0; i < count; i++) {
            kfree(nodes[i]);
        }
        kfree(nodes);

        return ENOMEM;
    }

    for (uint32_t i = 0; i < count; i++) {
        memcpy(&nodes_buffer[i], nodes[i], sizeof(node_t));
    }

    // copy nodes to user space
    if (copy_to_user((void *) user_buffer, nodes_buffer, needed_size)) {
        for (uint32_t i = 0; i < count; i++) {
            kfree(nodes[i]);
        }
        kfree(nodes);
        kfree(nodes_buffer);
        return EFAULT;
    }

    for (uint32_t i = 0; i < count; i++) {
        kfree(nodes[i]);
    }
    kfree(nodes);
    kfree(nodes_buffer);

    return 0;
}

uint32_t sys_load(uint32_t user_path, uint32_t dst, uint32_t unused1, uint32_t unused2, uint32_t unused3) {
    struct process_address_space *addr = load((char *) user_path, 0);
    if (!addr) {
        kfree(addr);
        return ENOENT;
    }

    if (copy_to_user((void *) dst, addr, sizeof(struct process_address_space))) {
        destroy_process(addr);
        kfree(addr);
        return EFAULT;
    }

    kfree(addr);
    return 0;
}

uint32_t sys_destroy_process(uint32_t pad, uint32_t unused1, uint32_t unused2, uint32_t unused3, uint32_t unused4) {
    struct process_address_space *s = (struct process_address_space *) pad;
    if (!s) {
        return EINVAL;
    }

    destroy_process(s);
    return 0;
}

uint32_t sys_create_task(uint32_t addr, uint32_t unused1, uint32_t unused2, uint32_t unused3, uint32_t unused4) {
    struct process_address_space *s = (struct process_address_space *) addr;
    if (!s) return EINVAL;

    int pid = create_user_task(s);
    
    return pid;
}

// returns the state of a task given its pid and places its return value in the pointer
uint32_t sys_get_state(uint32_t pid, uint32_t ptr, uint32_t unused1, uint32_t unused2, uint32_t unused3) {
    tcb_t *t = get_task(pid);

    if (t == NULL) {
        return ESRCH;
    }

    int ret = t->ret;
    task_state_t state = t->state;

    if (state == TASK_TERMINATED) {
        if (copy_to_user((void *) ptr, &ret, sizeof(task_state_t))) {
            return EFAULT;
        }
    }

    return state;
}

extern struct vbe_mode_info *vbe_info;
uint32_t sys_get_display_info(uint32_t vbe_info_ptr, uint32_t unused1, uint32_t unused2, uint32_t unused3, uint32_t unused4) {
    if (vbe_info_ptr == 0) {
        return EINVAL;
    }

    if (copy_to_user((void *) vbe_info_ptr, &vbe_info, sizeof(struct vbe_mode_info))) {
        return EFAULT;
    }

    struct vbe_mode_info *info = (struct vbe_mode_info *) vbe_info_ptr;
    info->framebuffer = LFB_VADDR;

    return 0;
}

extern uint32_t pitch;
extern tcb_t tasks[MAX_TASKS];
uint32_t sys_request_buffer(uint32_t width, uint32_t height, uint32_t unused1, uint32_t unused2, uint32_t unused3) {
    if (getpid() == -1) {
        return EPERM; // kernel cannot request a buffer, use buffer at LFB_VADDR instead
    }

    if (width == 0 || height == 0 || width > SCREEN_WIDTH || height > SCREEN_HEIGHT) {
        return EINVAL;
    }

    uint32_t size = width * height * 4; // 4 bytes per pixel (32 bpp)
    if (size > LFB_SIZE) {
        return ENOMEM;
    }

    uint32_t crt_task = getpid();

    if (tasks[crt_task].regs.eip == 0) return ESRCH;

    if ((tasks[crt_task].addr->has_buffer & 1) == 0) { // not allowed to have a buffer
        return EPERM;
    } else if ((tasks[crt_task].addr->has_buffer & 2) != 0) { // has already requested a buffer
        return EEXIST;
    }

    preempt_disable();

    // reference point
    uint32_t guard_page_addr = BIN_BASE_ADDR + (tasks[crt_task].addr->address_idx + 1) * MAX_PROCESS_SIZE - PROCESS_STACK_SIZE - 0x1000;

    if (size % 0x1000 != 0) {
        size += 0x1000 - (size % 0x1000); // page align
    }
    uint32_t buffer_start = guard_page_addr - size;
    tasks[crt_task].addr->buffer_start = buffer_start;
    tasks[crt_task].buf_w = width;
    tasks[crt_task].buf_h = height;

    // create a heap for the task (read asm/i386/cpu/task.c:149 for more info)
    uint32_t heap_start = BIN_BASE_ADDR + tasks[crt_task].addr->address_idx * MAX_PROCESS_SIZE + tasks[crt_task].addr->binary_size;
    if (heap_start % 0x1000 != 0) {
        heap_start += 0x1000 - (heap_start % 0x1000);
    }
    tasks[crt_task].heap = mkheap(
        heap_start,
        heap_start + KHEAP_INITIAL_SZ,
        buffer_start - 1,
        0, 0
    );

    tasks[crt_task].addr->has_buffer |= 2; // set bit 1

    serial_printf("Created buffer at 0x%x\n", buffer_start);

    preempt_enable();
    return buffer_start;
}

uint32_t sys_buffer_ready(uint32_t x, uint32_t y, uint32_t unused1, uint32_t unused2, uint32_t unused3) {
    if (getpid() == -1) return EPERM;

    if (x > SCREEN_WIDTH || y > SCREEN_HEIGHT) {
        return EINVAL;
    }

    if ((get_task(getpid())->addr->has_buffer & 1) == 0) return EPERM;

    add_buffer_to_queue(getpid(), x, y);

    return 0;
}

void syscall_handler(regs_t *regs) {
    uint32_t n = regs->eax;

    if (n < NUM_SYSCALLS && syscall_table[n]) {
        if (n != 0) {
            regs->eax = syscall_table[n](
                regs->ebx, regs->ecx, regs->edx, regs->esi, regs->edi
            );
        } else { // sys_exit has no return value
            syscall_table[0](
                regs->ebx, regs->ecx, regs->edx, regs->esi, regs->edi
            );
        }
    } else {
        regs->eax = EINVAL;
    }
}