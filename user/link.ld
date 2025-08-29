ENTRY(_start)

SECTIONS {
    .text : {
        *(.text._start)
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

    /DISCARD/ : {
        *(.comment)
        *(.note.gnu.build-id)
    }
}