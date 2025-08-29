#pragma once

#include <fs/skbdfs.h>

#define VFS_MAX_DEVICES 16

typedef struct vfs_device {
    /*
        for reading from /dev/ide (which has original signature (uint32_t, uint32_t, uint8_t *))
        we will be changing the signature to (file_t *, uint32_t, uint8_t *)
        in order to create a unified vfs for all devices.
        the ata_read_bytes/ata_write_bytes functions will take file->ptr_local as the drive offset.
    */
    uint32_t (*read)(struct file *file, uint32_t size, uint8_t *buffer);
    uint32_t (*write)(struct file *file, uint32_t size, uint8_t *buffer);
    struct file (*open)(char *path, uint8_t mode);
} PACKED vfs_device_t;

void init_vfs();

struct file *dev_by_idx(uint8_t idx);

struct file *vfs_open(char *path, uint8_t mode);
int vfs_read(struct file *file, uint32_t size, uint8_t *buffer);
int vfs_write(struct file *file, uint32_t size, uint8_t *buffer);