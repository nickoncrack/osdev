ENTRY(_start)

SECTIONS {
    .text : {
        *(.text)
    }

    .rodata ALIGN(4096): {
        *(.rodata)
    }

    .data ALIGN(4096): {
        *(.data)
    }

    .bss ALIGN(4096): {
        *(.bss COMMON)
    }
}