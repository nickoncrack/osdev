# osdev
"It works on my machine" <sub><sub>(No actually, everything I upload here has been tested on my vm and it works, so i wont be reading yalls issues unless its a feature request)</sub></sub>

<br>

## Features & TODOs
+ GDT/IDT ✅
+ PIT timer ✅
+ Paging ✅
+ Kernel heap memory allocator ✅
+ ACPI table parsing ✅
+ VESA mode (using LFB provided by grub) ✅
+ Text rendering with custom font support (using [ssfn.h](https://gitlab.com/bztsrc/scalable-font2/-/blob/master/ssfn.h?ref_type=heads)) ✅
+ Simple ATA driver with sector I/O ✅
+ Custom block based filesystem ([skbd-fs](https://github.com/dtxc/skbd-fs)) ✅
+ Multitasking and flat binary loader ✅
+ Keyboard and mouse drivers (wip)
+ Graphical interface :x:

<br>

## Notes
+ [Notes related to the filesystem](https://github.com/dtxc/skbd-fs)

### User task memory layout (8 MB process)
```py
# offset - (0x40000000 + i * 8 MB)
0x00000000   -> end_code        : code
end_code*    -> buffer_start**  : heap
buffer_start -> 0x007DFFFF      : guard page
0x007E0000   -> 0x007FFFFF      : stack (128 kb)
```

*page aligned
<br>
**Some tasks are not allowed to have a buffer, these tasks end up with a larger heap.

### Building & running
```sh
make
qemu-system-i386 -cdrom build/out/os-image.iso -hda image.bin -serial file:serial.log
```

### External binary example
Binary dependencies are currently hardcoded, so modifying `user/Makefile` is required.

```c
#include <stdio.h> // fprintf
#include <stdlib.h> // exit

__attribute__((section(".text._start")))
void _start() {
    fprintf(stdio, "Hello world!\n");
    exit(0);
}
```

<br>

## Third party software
While this operating system uses its own native tools, in also relies on [Scalable Screen Font 2.0](https://gitlab.com/bztsrc/scalable-font2/-/tree/master?ref_type=heads) for font rendering.
