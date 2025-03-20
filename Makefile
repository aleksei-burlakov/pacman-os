ASMFLAGS = -f elf
TARGET_CFLAGS = -m32 -fno-stack-protector -std=c99 -g -ffreestanding -nostdlib
BUILD_DIR = build/
ASM = nasm
LD = gcc

.PHONY: all floppy_image kernel bootloader clean always

all: always $(BUILD_DIR)/main_floppy.img

#
# Floppy image
#
$(BUILD_DIR)/main_floppy.img: $(BUILD_DIR)/stage1.bin $(BUILD_DIR)/stage2.bin $(BUILD_DIR)/kernel.bin
	@dd if=/dev/zero of=$@ bs=512 count=2880 >/dev/null
	@mkfs.fat -F 12 -n "FOOBAR" $@ >/dev/null
	@dd if=$(BUILD_DIR)/stage1.bin of=$@ conv=notrunc >/dev/null
	@mcopy -i $@ $(BUILD_DIR)/stage2.bin "::stage2.bin"
	@mcopy -i $@ $(BUILD_DIR)/kernel.bin "::kernel.bin"
	@mcopy -i $@ test.txt "::test.txt"
	@mmd -i $@ "::mydir"
	@mcopy -i $@ test.txt "::mydir/test.txt"
	@echo "--> Created: " $@

#
# Bootloader
#
$(BUILD_DIR)/stage1.bin: src/bootloader/stage1/boot.asm
	@$(ASM) $< -f bin -o $@
	@echo "--> Created  stage1.bin"

$(BUILD_DIR)/stage2/asm/entry.obj: src/bootloader/stage2/entry.asm
	@mkdir -p $(@D)
	@$(ASM) $(ASMFLAGS) -o $@ $<
	@echo "--> Compiled: " $<

$(BUILD_DIR)/stage2/asm/x86.obj: src/bootloader/stage2/x86.asm
	@mkdir -p $(@D)
	@$(ASM) $(ASMFLAGS) -o $@ $<
	@echo "--> Compiled: " $<

$(BUILD_DIR)/stage2/c/ctype.obj: src/bootloader/stage2/ctype.c
	@mkdir -p $(@D)
	$(CC) $(TARGET_CFLAGS) -Isrc/bootloader/stage2 -c -o $@ $<
	@echo "--> Compiled: " $<

$(BUILD_DIR)/stage2/c/disk.obj: src/bootloader/stage2/disk.c
	@mkdir -p $(@D)
	$(CC) $(TARGET_CFLAGS) -Isrc/bootloader/stage2 -c -o $@ $<
	@echo "--> Compiled: " $<

$(BUILD_DIR)/stage2/c/fat.obj: src/bootloader/stage2/fat.c
	@mkdir -p $(@D)
	$(CC) $(TARGET_CFLAGS) -Isrc/bootloader/stage2 -c -o $@ $<
	@echo "--> Compiled: " $<

$(BUILD_DIR)/stage2/c/main.obj: src/bootloader/stage2/main.c
	@mkdir -p $(@D)
	$(CC) $(TARGET_CFLAGS) -Isrc/bootloader/stage2 -c -o $@ $<
	@echo "--> Compiled: " $<

$(BUILD_DIR)/stage2/c/memory.obj: src/bootloader/stage2/memory.c
	@mkdir -p $(@D)
	$(CC) $(TARGET_CFLAGS) -Isrc/bootloader/stage2 -c -o $@ $<
	@echo "--> Compiled: " $<

$(BUILD_DIR)/stage2/c/stdio.obj: src/bootloader/stage2/stdio.c
	@mkdir -p $(@D)
	$(CC) $(TARGET_CFLAGS) -Isrc/bootloader/stage2 -c -o $@ $<
	@echo "--> Compiled: " $<

$(BUILD_DIR)/stage2/c/string.obj: src/bootloader/stage2/string.c
	@mkdir -p $(@D)
	$(CC) $(TARGET_CFLAGS) -Isrc/bootloader/stage2 -c -o $@ $<
	@echo "--> Compiled: " $<

STAGE2_OBJECTS = $(BUILD_DIR)/stage2/asm/entry.obj $(BUILD_DIR)/stage2/asm/x86.obj\
	$(BUILD_DIR)/stage2/c/ctype.obj $(BUILD_DIR)/stage2/c/disk.obj $(BUILD_DIR)/stage2/c/fat.obj\
	$(BUILD_DIR)/stage2/c/main.obj $(BUILD_DIR)/stage2/c/memory.obj $(BUILD_DIR)/stage2/c/stdio.obj\
	$(BUILD_DIR)/stage2/c/string.obj

$(BUILD_DIR)/stage2.bin: $(STAGE2_OBJECTS)
	@$(LD) -m32 -T src/bootloader/stage2/linker.ld -nostdlib -Wl,-Map=$(BUILD_DIR)/stage2.map -o $@ $^
	@echo "--> Created  stage2.bin"

#
# Kernel
#
$(BUILD_DIR)/kernel/asm/arch/i686/isr.obj: src/kernel/arch/i686/isr.asm
	@mkdir -p $(@D)
	@$(ASM) $(ASMFLAGS) -o $@ $<
	@echo "--> Compiled: " $<

$(BUILD_DIR)/kernel/asm/arch/i686/io.obj: src/kernel/arch/i686/io.asm
	@mkdir -p $(@D)
	@$(ASM) $(ASMFLAGS) -o $@ $<
	@echo "--> Compiled: " $<

$(BUILD_DIR)/kernel/asm/arch/i686/idt.obj: src/kernel/arch/i686/idt.asm
	@mkdir -p $(@D)
	@$(ASM) $(ASMFLAGS) -o $@ $<
	@echo "--> Compiled: " $<

$(BUILD_DIR)/kernel/asm/arch/i686/gdt.obj: src/kernel/arch/i686/gdt.asm
	@mkdir -p $(@D)
	@$(ASM) $(ASMFLAGS) -o $@ $<
	@echo "--> Compiled: " $<

$(BUILD_DIR)/kernel/c/stdio.obj: src/kernel/stdio.c
	@mkdir -p $(@D)
	$(CC) $(TARGET_CFLAGS) -Isrc/kernel -c -o $@ $<
	@echo "--> Compiled: " $<

$(BUILD_DIR)/kernel/c/memory.obj: src/kernel/memory.c
	@mkdir -p $(@D)
	$(CC) $(TARGET_CFLAGS) -Isrc/kernel -c -o $@ $<
	@echo "--> Compiled: " $<

$(BUILD_DIR)/kernel/c/main.obj: src/kernel/main.c
	@mkdir -p $(@D)
	$(CC) $(TARGET_CFLAGS) -Isrc/kernel -c -o $@ $<
	@echo "--> Compiled: " $<

$(BUILD_DIR)/kernel/c/debug.obj: src/kernel/debug.c
	@mkdir -p $(@D)
	$(CC) $(TARGET_CFLAGS) -Isrc/kernel -c -o $@ $<
	@echo "--> Compiled: " $<

$(BUILD_DIR)/kernel/c/pacman/engine.obj: src/kernel/pacman/engine.c
	@mkdir -p $(@D)
	$(CC) $(TARGET_CFLAGS) -Isrc/kernel -c -o $@ $<
	@echo "--> Compiled: " $<

$(BUILD_DIR)/kernel/c/hal/vfs.obj: src/kernel/hal/vfs.c
	@mkdir -p $(@D)
	$(CC) $(TARGET_CFLAGS) -Isrc/kernel -c -o $@ $<
	@echo "--> Compiled: " $<

$(BUILD_DIR)/kernel/c/hal/hal.obj: src/kernel/hal/hal.c
	@mkdir -p $(@D)
	$(CC) $(TARGET_CFLAGS) -Isrc/kernel -c -o $@ $<
	@echo "--> Compiled: " $<

$(BUILD_DIR)/kernel/c/arch/i686/vga_text.obj: src/kernel/arch/i686/vga_text.c
	@mkdir -p $(@D)
	$(CC) $(TARGET_CFLAGS) -Isrc/kernel -c -o $@ $<
	@echo "--> Compiled: " $<

$(BUILD_DIR)/kernel/c/arch/i686/isrs_gen.obj: src/kernel/arch/i686/isrs_gen.c
	@mkdir -p $(@D)
	$(CC) $(TARGET_CFLAGS) -Isrc/kernel -c -o $@ $<
	@echo "--> Compiled: " $<

$(BUILD_DIR)/kernel/c/arch/i686/isr.obj: src/kernel/arch/i686/isr.c
	@mkdir -p $(@D)
	$(CC) $(TARGET_CFLAGS) -Isrc/kernel -c -o $@ $<
	@echo "--> Compiled: " $<

$(BUILD_DIR)/kernel/c/arch/i686/irq.obj: src/kernel/arch/i686/irq.c
	@mkdir -p $(@D)
	$(CC) $(TARGET_CFLAGS) -Isrc/kernel -c -o $@ $<
	@echo "--> Compiled: " $<

$(BUILD_DIR)/kernel/c/arch/i686/io.obj: src/kernel/arch/i686/io.c
	@mkdir -p $(@D)
	$(CC) $(TARGET_CFLAGS) -Isrc/kernel -c -o $@ $<
	@echo "--> Compiled: " $<

$(BUILD_DIR)/kernel/c/arch/i686/gdt.obj: src/kernel/arch/i686/gdt.c
	@mkdir -p $(@D)
	$(CC) $(TARGET_CFLAGS) -Isrc/kernel -c -o $@ $<
	@echo "--> Compiled: " $<

$(BUILD_DIR)/kernel/c/arch/i686/idt.obj: src/kernel/arch/i686/idt.c
	@mkdir -p $(@D)
	$(CC) $(TARGET_CFLAGS) -Isrc/kernel -c -o $@ $<
	@echo "--> Compiled: " $<

$(BUILD_DIR)/kernel/c/arch/i686/e9.obj: src/kernel/arch/i686/e9.c
	@mkdir -p $(@D)
	$(CC) $(TARGET_CFLAGS) -Isrc/kernel -c -o $@ $<
	@echo "--> Compiled: " $<

$(BUILD_DIR)/kernel/c/arch/i686/i8259.obj: src/kernel/arch/i686/i8259.c
	@mkdir -p $(@D)
	$(CC) $(TARGET_CFLAGS) -Isrc/kernel -c -o $@ $<
	@echo "--> Compiled: " $<

KERNEL_OBJECTS = $(BUILD_DIR)/kernel/asm/arch/i686/isr.obj $(BUILD_DIR)/kernel/asm/arch/i686/io.obj\
	$(BUILD_DIR)/kernel/asm/arch/i686/idt.obj $(BUILD_DIR)/kernel/asm/arch/i686/gdt.obj\
	$(BUILD_DIR)/kernel/c/stdio.obj $(BUILD_DIR)/kernel/c/memory.obj $(BUILD_DIR)/kernel/c/main.obj\
	$(BUILD_DIR)/kernel/c/debug.obj $(BUILD_DIR)/kernel/c/pacman/engine.obj\
	$(BUILD_DIR)/kernel/c/hal/vfs.obj $(BUILD_DIR)/kernel/c/hal/hal.obj\
	$(BUILD_DIR)/kernel/c/arch/i686/vga_text.obj $(BUILD_DIR)/kernel/c/arch/i686/isrs_gen.obj\
	$(BUILD_DIR)/kernel/c/arch/i686/isr.obj $(BUILD_DIR)/kernel/c/arch/i686/irq.obj\
	$(BUILD_DIR)/kernel/c/arch/i686/io.obj $(BUILD_DIR)/kernel/c/arch/i686/gdt.obj\
	$(BUILD_DIR)/kernel/c/arch/i686/idt.obj $(BUILD_DIR)/kernel/c/arch/i686/e9.obj\
	$(BUILD_DIR)/kernel/c/arch/i686/i8259.obj

arch/i686/isrs_gen.c src/kernel/arch/i686/isrs_gen.inc:
	build_scripts/generate_isrs.sh $@
	@echo "src/kernel/arch/i686/isrs_gen.inc --> generated"

$(BUILD_DIR)/kernel.bin: $(KERNEL_OBJECTS)
	@$(LD) -m32 -T src/kernel/linker.ld -nostdlib -Wl,-Map=$(BUILD_DIR)/kernel.map -o $@ $^
	@echo "--> Created:  kernel.bin"

#
# Always
#
always:
	@mkdir -p $(BUILD_DIR)

#
# Run
#
run: $(BUILD_DIR)/main_floppy.img
	qemu-system-i386 -debugcon stdio -fda $(BUILD_DIR)/main_floppy.img

#
# Debug
#
debug:
	bochs -f bochs_config

#
# Clean
#
clean:
	@rm -rf $(BUILD_DIR)/*
