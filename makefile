# Compiler & linker
ASM           = nasm
LIN           = ld
CC            = gcc

# Directory
SOURCE_FOLDER = src
INTERRUPT_FOLDER = interrupt
KEYBOARD_FOLDER = keyboard
FILESYSTEM_FOLDER = filesystem
FRAMEBUFFER_FOLDER = framebuffer
# SOURCE_FOLDER = bin/iso/boot/grub
OUTPUT_FOLDER = bin
ISO_NAME      = OSyikkk
# DISK 
DISK_NAME = storage

# Flags
WARNING_CFLAG = -Wall -Wextra -Werror
DEBUG_CFLAG   = -ffreestanding -fshort-wchar -g
STRIP_CFLAG   = -nostdlib -nostdinc -fno-builtin -fno-stack-protector -nostartfiles -nodefaultlibs
CFLAGS        = $(DEBUG_CFLAG) $(WARNING_CFLAG) $(STRIP_CFLAG) -m32 -c -I$(SOURCE_FOLDER)
AFLAGS        = -f elf32 -g -F dwarf
LFLAGS        = -T $(SOURCE_FOLDER)/linker.ld -melf_i386


run: all
	# @qemu-system-i386 -s -cdrom $(OUTPUT_FOLDER)/$(ISO_NAME).iso
	@qemu-system-i386 -s -drive file=$(OUTPUT_FOLDER)/$(DISK_NAME).bin,format=raw,if=ide,index=0,media=disk -cdrom $(OUTPUT_FOLDER)/$(ISO_NAME).iso
debug: all
	@qemu-system-i386 -s -S -drive file=$(OUTPUT_FOLDER)/$(DISK_NAME).bin,format=raw,if=ide,index=0,media=disk -cdrom $(OUTPUT_FOLDER)/$(ISO_NAME).iso
all: build
build: iso
clean:
	rm -rf *.o *.iso $(OUTPUT_FOLDER)/kernel
disk:
	@qemu-img create -f raw $(OUTPUT_FOLDER)/$(DISK_NAME).bin 4M	


kernel:
	@$(ASM) $(AFLAGS) $(SOURCE_FOLDER)/kernel_loader.s -o $(OUTPUT_FOLDER)/kernel_loader.o
	@$(ASM) $(AFLAGS) $(SOURCE_FOLDER)/$(INTERRUPT_FOLDER)/intsetup.s -o $(OUTPUT_FOLDER)/intsetup.o
# TODO: Compile C file with CFLAGS
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/kernel.c -o $(OUTPUT_FOLDER)/kernel.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/stdmem.c -o $(OUTPUT_FOLDER)/stdmem.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/portio.c -o $(OUTPUT_FOLDER)/portio.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/gdt.c -o $(OUTPUT_FOLDER)/gdt.o

	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/$(FRAMEBUFFER_FOLDER)/framebuffer.c -o $(OUTPUT_FOLDER)/framebuffer.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/$(INTERRUPT_FOLDER)/idt.c -o $(OUTPUT_FOLDER)/idt.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/$(INTERRUPT_FOLDER)/interrupt.c -o $(OUTPUT_FOLDER)/interrupt.o
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/$(KEYBOARD_FOLDER)/keyboard.c -o $(OUTPUT_FOLDER)/keyboard.o	
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/$(FILESYSTEM_FOLDER)/disk.c -o $(OUTPUT_FOLDER)/disk.o	
	@$(CC) $(CFLAGS) $(SOURCE_FOLDER)/$(FILESYSTEM_FOLDER)/fat32.c -o $(OUTPUT_FOLDER)/fat32.o	
	
	
	@$(LIN) $(LFLAGS) bin/*.o -o $(OUTPUT_FOLDER)/kernel
	@echo Linking object files and generate elf32...
	@rm -f *.o

iso: kernel
	# @rm -r $(OUTPUT_FOLDER)/iso/
	@mkdir -p $(OUTPUT_FOLDER)/iso/boot/grub
	@cp $(OUTPUT_FOLDER)/kernel     $(OUTPUT_FOLDER)/iso/boot/
	@cp other/grub1                 $(OUTPUT_FOLDER)/iso/boot/grub/
	@cp $(SOURCE_FOLDER)/menu.lst   $(OUTPUT_FOLDER)/iso/boot/grub/
# TODO: Create ISO image
	@genisoimage -R            \
	-b boot/grub/grub1         \
	-no-emul-boot              \
	-boot-load-size 4          \
	-A $(ISO_NAME)             \
	-input-charset utf8        \
	-quiet                     \
	-boot-info-table           \
	-o $(OUTPUT_FOLDER)/$(ISO_NAME).iso              \
	$(OUTPUT_FOLDER)/iso
	
