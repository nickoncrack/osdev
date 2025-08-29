MBOOT_PAGE_ALIGN equ 1 << 0
MBOOT_MEM_INFO   equ 1 << 1
MBOOT_VIDEO_MODE equ 1 << 2

MAGIC      equ 0x1BADB002
FLAGS      equ MBOOT_PAGE_ALIGN | MBOOT_MEM_INFO | MBOOT_VIDEO_MODE
CHECKSUM   equ -(MAGIC + FLAGS)

bits 32

mboot:
	dd MAGIC
	dd FLAGS
	dd CHECKSUM

	times 5 dd 0

	dd 0 ; LFB
	dd 800 ; width
	dd 600  ; height
	dd 32   ; bpp
	
	times 2 dd 0 ; padding

global start
extern kernel_init

start:
	push esp
	push ebx

	cli
	call kernel_init
	jmp $