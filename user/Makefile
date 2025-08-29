CCFLAGS = -I ./include -m32 -fPIC -ffreestanding -nostdlib -nostartfiles -nodefaultlibs -fno-stack-protector -Wall -Wextra -O2

SRC_TEST := lib/string.o lib/stdio.o lib/stdlib.o src/test.o
OBJ_TEST := $(SRC_TEST:.c=.o)

SRC_TTY := lib/string.o lib/stdio.o lib/stdlib.o lib/task.o lib/exec.o src/tty.o
OBJ_TTY := $(SRC_TTY:.c=.o)

SRC_GUI := lib/stdlib.c lib/string.o src/gui.c
OBJ_GUI := $(SRC_GUI:.c=.o)

build: out/test.o out/tty.o out/gui.o
	rm -rf $(OBJ_TEST) $(OBJ_TTY) $(OBJ_GUI) out/test.o out/tty.o out/gui.o

out/test.o: $(OBJ_TEST)
	$(CC) $(CCFLAGS) -o $@ $^
	ld -m elf_i386 -o $@ -T link.ld $^
	objcopy -O binary $@ out/test

out/tty.o: $(OBJ_TTY)
	$(CC) $(CCFLAGS) -o $@ $^
	ld -m elf_i386 -o $@ -T link.ld $^
	objcopy -O binary $@ out/tty

out/gui.o: $(OBJ_GUI)
	$(CC) $(CCFLAGS) -o $@ $^
	ld -m elf_i386 -o $@ -T link.ld $^
	objcopy -O binary $@ out/gui

%.o: %.c
	$(CC) $(CCFLAGS) -c $< -o $@