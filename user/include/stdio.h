#pragma once

#include <stdint.h>

typedef struct {
    uint32_t magic;

    uint8_t flags;
    uint64_t size; // for FS_DIR: size = n of children
    uint32_t first_block;
    uint8_t mode : 6;

    char name[32];

    char reserved[78];
} __attribute__((packed)) node_t;

typedef struct vfs_device {
    int (*read)(struct file *file, uint32_t size, uint8_t *buffer);
    int (*write)(struct file *file, uint32_t size, uint8_t *buffer);
    struct file (*open)(char *path, uint8_t mode);
} __attribute__((packed)) vfs_device_t;

typedef struct file {
    uint8_t type;

    union {
        node_t *node;
        struct vfs_device *device;
    };

    uint8_t is_locked;

    uint32_t ptr_local; // offset inside the file
    uint64_t ptr_global; // offset inside the drive
    uint8_t mode : 3;
} file_t;

#define stdout ((file_t *) 1)
#define serial ((file_t *) 2)
#define stdin  ((file_t *) 3)

#define STDOUT_CLEAR "\033[H\033[J"


void puts(const char *s);
void fprintf(file_t *f, const char *s, ...);
void scanf(char *dst, uint32_t size);

void sprintf(char *dst, const char *s, ...);

uint32_t listdir(char *dir, node_t **dst);

void serial_puts(char *s);