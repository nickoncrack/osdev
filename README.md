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
+ Multitasking and ELF loader* ❌
+ Keyboard and mouse drivers ❌
+ Graphical interface ❌

<sub>Multitasking and flat binary loader lowkey complete but im gonna upload them all together when i finish user mode</sub>
<br>

## Notes
+ The LFB is originally placed at `0xFD000000` and is mapped at VA = `0xD0000000`, mapping it with PA = VA causes issues which I may resolve in the future.
+ The PIT timer is inaccurate, an IRQ is fired every 0.999847 ms, resulting in a 153 ns error every second. (i.e. calling `pit_sleep(10000)` will finish 1530 ns earlier)
+ The ATA driver is currently implemented using programmed I/O which is significantly slower than direct memory access (DMA).
+ [Notes related to the filesystem](https://github.com/dtxc/skbd-fs)

<br>

## Third party software
While this operating system uses its own native tools, in also relies on [Scalable Screen Font 2.0](https://gitlab.com/bztsrc/scalable-font2/-/tree/master?ref_type=heads) for font rendering.
