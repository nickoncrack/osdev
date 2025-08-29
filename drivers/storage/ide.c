#include <string.h>

#include <hw/ata.h>
#include <asm/io.h>

#include <fs/skbdfs.h>

static void ata_select_device(uint8_t drive, uint32_t lba) {
    outb(ATA_PRIMARY_IO_BASE + 6, 0xE0 | (drive << 4) | ((lba >> 24) & 0x0F));
    outb(ATA_PRIMARY_IO_BASE + 2, 1);
    outb(ATA_PRIMARY_IO_BASE + 3, (uint8_t) (lba & 0xFF));
    outb(ATA_PRIMARY_IO_BASE + 4, (uint8_t) ((lba >> 8) & 0xFF));
    outb(ATA_PRIMARY_IO_BASE + 5, (uint8_t) ((lba >> 16) & 0xFF));
}

// TODO: error checking
static void ata_polling() {
    // wait for BSY to be 0
    while (inb(ATA_PRIMARY_IO_BASE + 7) & 0x80); // 0x80: busy
}

static int ata_read_sector(uint8_t drive, uint32_t lba, uint8_t *buffer) {
    ata_polling();
    ata_select_device(drive, lba);
    outb(ATA_PRIMARY_IO_BASE + 7, ATA_CMD_READ);
    ata_polling();

    for (int i = 0; i < ATA_SECTOR_SIZE / 2; i++) {
        uint16_t data = inw(ATA_PRIMARY_IO_BASE);
        buffer[i * 2] = (uint8_t) (data & 0xFF);
        buffer[i * 2 + 1] = (uint8_t) ((data >> 8) & 0xFF);
    }

    return 0;
}

static int ata_write_sector(uint8_t drive, uint32_t lba, const uint8_t *buffer) {
    ata_polling();
    ata_select_device(drive, lba);
    outb(ATA_PRIMARY_IO_BASE + 7, ATA_CMD_WRITE);
    ata_polling();

    for (int i = 0; i < ATA_SECTOR_SIZE / 2; i++) {
        uint16_t data = ((uint16_t) buffer[i * 2 + 1] << 8) | buffer[i * 2];
        outw(ATA_PRIMARY_IO_BASE, data);
    }

    // flush cache
    outb(ATA_PRIMARY_IO_BASE + 7, 0xE7);

    return 0;
}

// assume drive 0
uint32_t ata_read_bytes(struct file *file, uint32_t size, uint8_t *buffer) {
    uint32_t offset = file->ptr_global;
    uint32_t start_lba = offset / ATA_SECTOR_SIZE;
    uint32_t end_lba = (offset + size - 1) / ATA_SECTOR_SIZE;
    uint32_t sector_offset = offset % ATA_SECTOR_SIZE;
    uint32_t remaining_bytes = size;
    uint8_t sector_buffer[ATA_SECTOR_SIZE];

    for (uint32_t lba = start_lba; lba <= end_lba; lba++) {
        if (ata_read_sector(0, lba, sector_buffer) != 0) {
            return -1;
        }

        uint32_t copy_offset = (lba == start_lba) ? sector_offset : 0;
        uint32_t copy_size = (remaining_bytes < (ATA_SECTOR_SIZE - copy_offset)) ? remaining_bytes : (ATA_SECTOR_SIZE - copy_offset);

        memcpy(buffer, sector_buffer + copy_offset, copy_size);

        buffer += copy_size;
        remaining_bytes -= copy_size;
    }

    return 0;
}

uint32_t ata_write_bytes(struct file *file, uint32_t size, uint8_t *buffer) {
    uint32_t offset = file->ptr_global;
    uint32_t start_lba = offset / ATA_SECTOR_SIZE;
    uint32_t end_lba = (offset + size - 1) / ATA_SECTOR_SIZE;
    uint32_t sector_offset = offset % ATA_SECTOR_SIZE;
    uint32_t remaining_bytes = size;
    uint8_t sector_buffer[ATA_SECTOR_SIZE];

    for (uint32_t lba = start_lba; lba <= end_lba; lba++) {
        uint32_t write_offset = (lba == start_lba) ? sector_offset : 0;
        uint32_t write_size = (remaining_bytes < (ATA_SECTOR_SIZE - write_offset)) ? remaining_bytes : (ATA_SECTOR_SIZE - write_offset);

        memcpy(sector_buffer + write_offset, buffer, write_size);

        if (ata_write_sector(0, lba, sector_buffer) != 0) {
            return -1; // I/O error
        }

        buffer += write_size;
        remaining_bytes -= write_size;
    }

    return 0;
}

// returns the drive size in KiB
uint32_t get_drive_size(uint8_t drive) {
    uint16_t buffer[256];
    uint32_t ret = 0;

    ata_polling();
    ata_select_device(drive, 0);
    
    outb(ATA_PRIMARY_IO_BASE + 7, ATA_CMD_IDENTIFY);
    ata_polling();

    for (int i = 0; i < 256; i++) {
        buffer[i] = inw(ATA_PRIMARY_IO_BASE);
    }

    ret = (buffer[60] | (buffer[61] << 16)) / 2;
    return ret;
}