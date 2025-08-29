#include <fs/vfs.h>
#include <hw/ata.h>
#include <asm/io.h>

#include <stdio.h>
#include <string.h>
#include <mm/kheap.h>
#include <video/vga.h>
#include <video/vbe.h>

struct mount {
    const char *path;
    vfs_device_t *dev;

    uint8_t flags;
};

static struct mount mounts[VFS_MAX_DEVICES];

extern uint32_t read_buffer(file_t *unused, uint32_t size, uint8_t *dst);

static uint32_t __print_int(file_t *file, uint32_t size, uint8_t *buffer) {
    #ifdef VESA_MODE
    vesa_puts((char *) buffer);
    #else
    puts((char *) buffer);
    #endif
    return size;
}

static uint32_t __serial_print_int(file_t *file, uint32_t size, uint8_t *buffer) {
    serial_puts((char *) buffer);
    return size;
}

struct file *dev_by_idx(uint8_t idx) {
    if (idx >= VFS_MAX_DEVICES || mounts[idx].dev == NULL) {
        return NULL;
    } 

    struct file *ret = (struct file *) kmalloc(sizeof(struct file));

    ret->type = FILE_DEVICE;
    ret->device = mounts[idx].dev;
    ret->mode = mounts[idx].flags;
    ret->ptr_global = ret->ptr_local = 0;
    ret->is_locked = 0;

    return ret;
}

void init_vfs() {
    memset(&mounts, 0, sizeof(mounts));

    mounts[0].dev = (vfs_device_t *) kmalloc(sizeof(vfs_device_t));
    mounts[0].path = "/dev/ide";
    mounts[0].dev->open = NULL;
    mounts[0].dev->read = ata_read_bytes;
    mounts[0].dev->write = ata_write_bytes;
    mounts[0].flags = MODE_R | MODE_W;
    
    mounts[1].dev = (vfs_device_t *) kmalloc(sizeof(vfs_device_t));
    mounts[1].path = "/dev/stdout";
    mounts[1].dev->open = NULL;
    mounts[1].dev->read = NULL;
    mounts[1].dev->write = __print_int;
    mounts[1].flags = MODE_W;

    mounts[2].dev = (vfs_device_t *) kmalloc(sizeof(vfs_device_t));
    mounts[2].path = "/dev/serial";
    mounts[2].dev->open = NULL;
    mounts[2].dev->read = NULL;
    mounts[2].dev->write = __serial_print_int;
    mounts[2].flags = MODE_W;

    mounts[3].dev = (vfs_device_t *) kmalloc(sizeof(vfs_device_t));
    mounts[3].path = "/dev/stdin";
    mounts[3].dev->open = NULL;
    mounts[3].dev->read = read_buffer;
    mounts[3].dev->write = NULL;
    mounts[3].flags = MODE_R;
}

struct file *vfs_open(char *path, uint8_t mode) {
    uint8_t type = FILE_NORMAL;

    // iterate all mounted devices to check if path is of a device
    int i = 0;
    for (i = 0; i < VFS_MAX_DEVICES; i++) {
        if (strcmp(path, mounts[i].path)) {
            type = FILE_DEVICE;
            break;
        }
    }

    struct file *ret;

    if (type == FILE_NORMAL) {
        ret = fopen(path, mode);
        ret->type = FILE_NORMAL;

        return ret;
    }

    return dev_by_idx(i);
}

int vfs_write(struct file *file, uint32_t size, uint8_t *buffer) {
    if (file->type == FILE_NORMAL) {
        return fwrite(file, size, buffer);
    } else if (file->type == FILE_DEVICE) {
        if (file->device->write == NULL) {
            return -1; // Operation not permitted
        }

        return file->device->write(file, size, buffer);
    }

    return -2; // Invalid data
}

int vfs_read(struct file *file, uint32_t size, uint8_t *buffer) {
    if (file->type == FILE_NORMAL) {
        return fread(file, size, buffer);
    } else if (file->type == FILE_DEVICE) {
        if (file->device->read == NULL) {
            return -1;
        }
        
        return file->device->read(file, size, buffer);
    }

    return -2;
}