#pragma once

#include <common.h>

#define BIOS_BASE_ADDR 0x000E0000
#define BIOS_MAX_ADDR  0x000FFFFF

#define ACPI_HEAD_DEF struct ACPISTDHeader header

#define HPET_CAP_ID        0x00
#define HPET_CONFIG        0x10
#define HPET_INTR_STATUS   0x20
#define HPET_COUNTER       0xF0
#define HPET_TIMER_OFFSET  0x100
#define HPET_TIMER_SIZE    0x20

struct GenericAddressStructure {
    uint8_t AddressSpace;
    uint8_t BitWidth;
    uint8_t BitOffset;
    uint8_t AccessSize;

    uint64_t addr;
} __attribute__((packed));

struct ACPISTDHeader {
    char signature[4];

    uint32_t len;
    uint8_t revision;
    uint8_t checksum;

    char OEMID[6];
    uint64_t OEMTableID;

    uint32_t OEMRevision;
    uint32_t creatorId;
    uint32_t creatorRev;
} __attribute__((packed));

struct FADT {
    ACPI_HEAD_DEF;
    uint32_t firmwareCtrl;
    uint32_t DSDT;

    // field used in ACPI 1.0
    uint8_t reserved;

    uint8_t preferredPowerManagmentProfile;
    uint16_t SCI_interrupt;
    uint32_t SMI_commandPort;
    uint8_t acpiEnable;
    uint8_t acpiDisable;
    uint8_t S4BIOS_REQ;
    uint8_t PSTATE_CONTROL;
    uint32_t PM1aEventBlock;
    uint32_t PM1bEventBlock;
    uint32_t PM1aControlBlock;
    uint32_t PM1bControlBlock;
    uint32_t PM2ControlBlock;
    uint32_t PMTimerBlock;
    uint32_t GPE0Block;
    uint32_t GPE1Block;
    uint8_t PM1EventLength;
    uint8_t PM1ControlLength;
    uint8_t PM2ControlLength;
    uint8_t PMTimerLength;
    uint8_t GPE0Length;
    uint8_t GPE1Length;
    uint8_t GPE1Base;
    uint8_t CStateControl;
    uint16_t WorstC2Latency;
    uint16_t WorstC3Latency;
    uint16_t FlushSize;
    uint16_t FlushStride;
    uint8_t DutyOffset;
    uint8_t DutyWidth;
    uint8_t DayAlarm;
    uint8_t MonthAlarm;
    uint8_t Century;

    // reserved in ACPI 1.0, in use since ACPI 2.0
    uint16_t BootArchitectureFlags;

    uint8_t reserved2;
    uint32_t flags;

    struct GenericAddressStructure ResetReg;

    uint8_t ResetValue;
    uint8_t reserved3[3];

    // 64 bit pointers, available in ACPI 2.0+
    uint64_t X_FirmwareControl;
    uint64_t X_DSDT;

    struct GenericAddressStructure X_PM1aEventBlock;
    struct GenericAddressStructure X_PM1bEventBlock;
    struct GenericAddressStructure X_PM1aControlBlock;
    struct GenericAddressStructure X_PM1bControlBlock;
    struct GenericAddressStructure X_PM2ControlBlock;
    struct GenericAddressStructure X_PMTimerBlock;
    struct GenericAddressStructure X_GPE0Block;
    struct GenericAddressStructure X_GPE1Block;
} __attribute__((packed));

struct RSDP {
   char signature[8];
   uint8_t checksum;
   char OEMID[6];
   uint8_t revision;
   uint32_t RSDTAddress;
} __attribute__((packed));

struct RSDT {
    ACPI_HEAD_DEF;
    uint32_t pointerToOtherSDT[]; // size: (h.length - sizeof(h)) / 4
} __attribute__((packed));

struct HPET { // HPET Description Table
    ACPI_HEAD_DEF;
    uint8_t hardware_rev_id;
    uint8_t comparator_count : 5;
    uint8_t counter_size : 1;
    uint8_t reserved : 1;
    uint8_t legacy_replacement : 1;
    uint16_t pci_vendor_id;
    struct GenericAddressStructure address;
    uint8_t hpet_number;
    uint16_t minimum_tick;
    uint8_t page_protection;
} __attribute__((packed));

void init_acpi();