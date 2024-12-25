#include <hw/ata.h>
#include <asm/io.h>

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

int ata_read_sector(uint8_t drive, uint32_t lba, uint8_t *buffer) {
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

int ata_write_sector(uint8_t drive, uint32_t lba, const uint8_t *buffer) {
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
