C_SOURCES = $(shell find . -name "*.c" -not -path "./fs_generator/*" -not -path "./user/*")
OBJ_FILES = ${C_SOURCES:.c=.o \
	arch/i386/boot/entry.o \
	arch/i386/asm/dt.o \
	arch/i386/asm/int.o \
	arch/i386/asm/page.o \
	fonts/console.o \
}

CCFLAGS = -I ./include -I ./arch/i386/include -g -m32 -ffreestanding -nostdlib -fno-builtin -fno-stack-protector -no-pie -fno-pic -Wall -Wextra -O3
KERNELBIN = ./build/kernel.bin
ISO_ROOT = ./build/iso-root
ISO = ./build/out/os-image.iso

.PHONY: user

all: prepare user fs_gen $(ISO)

user:
	cd user && make

prepare:
	@mkdir -p ./build/out ./build/iso-root ./build/obj

fs_gen: user
	mkdir -p ./fs_generator/rootfs/bin
	cp ./user/out/* ./fs_generator/rootfs/bin/
	cd fs_generator && make

kernel.bin: ${OBJ_FILES}
	ld -m elf_i386 -o ./build/$(notdir $@) -Tlinker.ld ./build/obj/entry.o $(shell find ./build/obj/ -name "*.o" ! -name "entry.o")

$(ISO_ROOT)/boot/grub/grub.cfg:
	@mkdir -p $(dir $@)
	echo 'set timeout=5' > $@
	echo 'set default=0' >> $@
	echo '' >> $@
	echo 'menuentry "Os" {' >> $@
	echo '    multiboot /boot/kernel.bin' >> $@
	echo '    boot' >> $@
	echo '}' >> $@

$(ISO_ROOT)/boot/kernel.bin: kernel.bin
	@mkdir -p $(dir $@)
	cp $(KERNELBIN) $@

$(ISO): $(ISO_ROOT)/boot/grub/grub.cfg $(ISO_ROOT)/boot/kernel.bin
	grub-mkrescue -o $(ISO) $(ISO_ROOT)

%.o: %.c
	$(CC) $(CCFLAGS) -c $< -o ./build/obj/$(notdir $@)

%.o: %.asm
	nasm $< -f elf -o ./build/obj/$(notdir $@)

%.o: %.sfn
	cd fonts && objcopy -O elf32-i386 -B i386 -I binary console.sfn console.o && mv console.o ../build/obj/console.o

clean:

	$(RM) -r ./build
