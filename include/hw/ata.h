#pragma once

#include <common.h>
#include <fs/skbdfs.h>

#define ATA_PRIMARY_IO_BASE 0x1F0
#define ATA_PRIMARY_CTRL_BASE 0x3F6

#define ATA_CMD_READ 0x20
#define ATA_CMD_WRITE 0x30
#define ATA_CMD_IDENTIFY 0xEC

#define ATA_SECTOR_SIZE 512

uint32_t ata_read_bytes(struct file *file, uint32_t size, uint8_t *buffer);
uint32_t ata_write_bytes(struct file *file, uint32_t size, uint8_t *buffer);
uint32_t get_drive_size(uint8_t drive);