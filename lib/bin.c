#include <bin.h>
#include <assert.h>
#include <asm/io.h>
#include <int/task.h>
#include <mm/paging.h>
#include <fs/skbdfs.h>

_Bool addresses[MAX_PROCESS]; // 8 mb per process

int16_t find_index() {
    // find next available index (simple version, as every binary wont exceed 8 mb)
    for (int i = 0; i < MAX_PROCESS; i++) {
        if (addresses[i] == 0) {
            addresses[i] = 1;
            return i;
        }
    }

    return -1;
}

void destroy_process(struct process_address_space *s) {
    if (!s) return;

    // free memory
    uint32_t base_addr = BIN_BASE_ADDR + s->address_idx * MAX_PROCESS_SIZE;

    pagedir_t *dir;
    if (getpid() == -1) {
        dir = kernel_dir;
    } else {
        dir = get_task(getpid())->page_dir;
    }

    for (uint32_t i = base_addr; i < base_addr + MAX_PROCESS_SIZE - 1; i += 0x1000) {
        free_frame(get_page(i, 0, dir));
    }

    addresses[s->address_idx] = 0; // mark as free

    return;
}

// returns the address space of the loaded binary, which will be placed inside the tcb.
// entry point = BIN_BASE_ADDR + i * 8 MB
struct process_address_space *load(char *path, _Bool has_buffer) {
    /*
        task without buffer:
        offset - (BIN_BASE_ADDR + i * 8 MB)
        0x00000000 -> end_code   : code
        end_code*  -> 0x007DEFFF : heap
        0x007DF000 -> 0x007DFFFF : guard page (stack top - 128 kb)
        0x007E0000 -> 0x007FFFFF : stack (128 kb)

        *page aligned
    */

    // all tasks will be written into the kernel directory, then switched to their own
    switch_page_dir(kernel_dir);

    file_t *file = fopen(path, FMODE_R);

    if (!file) {
        #ifdef DEBUG
        serial_printf("load: fopen failed for %s\n", path);
        #endif
        return NULL;
    }

    int i = find_index();
    if (i == -1) { // out of memory
        #ifdef DEBUG
        serial_printf("load: out of memory\n");
        #endif

        fclose(file);
        return NULL;
    }

    struct process_address_space *s = (void *) kmalloc(sizeof(struct process_address_space));

    if (!s) {
        #ifdef DEBUG
        serial_printf("load: kmalloc failed\n");
        #endif

        fclose(file);
        return NULL;
    }

    s->address_idx = i;
    s->binary_size = file->node->size;

    uint32_t base_addr = BIN_BASE_ADDR + i * MAX_PROCESS_SIZE; // 8 MB per process
    #ifdef DEBUG
    serial_printf("load: Loading binary %s at 0x%x\n", path, base_addr);
    #endif

    // map memory
    for (uint32_t j = base_addr; j < base_addr + MAX_PROCESS_SIZE - 1; j += 0x1000) {
        alloc_frame(get_page(j, 1, kernel_dir), 0, 1);
    }

    fread(file, file->node->size, (uint8_t *) base_addr);

    fclose(file);

    if (getpid() != -1) {
        switch_page_dir(get_task(getpid())->page_dir);
    }

    s->has_buffer = 1; // set bit 0

    return s;
}