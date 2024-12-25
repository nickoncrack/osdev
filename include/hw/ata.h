#pragma once

#include <common.h>

#define ATA_PRIMARY_IO_BASE 0x1F0
#define ATA_PRIMARY_CTRL_BASE 0x3F6
#define ATA_CMD_READ 0x20
#define ATA_CMD_WRITE 0x30
#define ATA_SECTOR_SIZE 512

int ata_read_sector(uint8_t drive, uint32_t lba, uint8_t* buffer);
int ata_write_sector(uint8_t drive, uint32_t lba, const uint8_t *buffer);
uint32_t get_drive_size(uint8_t drive);
