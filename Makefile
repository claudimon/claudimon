
CC      = gcc
CFLAGS  = -m32                   \
           -ffreestanding        \
           -fno-stack-protector  \
           -fno-pic              \
           -mno-red-zone         \
           -nostdlib             \
           -nostdinc             \
           -I include            \
           -O2                   \
           -Wall -Wextra

NASM    = nasm
NASMFLAGS = -f elf32   # Output 32-bit ELF object file

LD      = ld
LDFLAGS = -m elf_i386 -T linker.ld --nmagic

# ---- Files ----
BOOT_OBJ = boot/boot.o
C_SRCS   = src/kernel.c src/vga.c
C_OBJS   = $(C_SRCS:.c=.o)
OBJS     = $(BOOT_OBJ) $(C_OBJS)
KERNEL   = mykernel.bin
ISO      = mykernel.iso

# ============================================
# BUILD TARGETS
# ============================================

# Default: build everything
all: $(ISO)

# 1. Assemble boot.asm → boot.o
boot/boot.o: boot/boot.asm
	$(NASM) $(NASMFLAGS) -o $@ $<

# 2. Compile C files → .o files
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# 3. Link everything into the kernel binary
$(KERNEL): $(OBJS) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

# 4. Build a bootable ISO with GRUB
$(ISO): $(KERNEL)
	cp $(KERNEL) iso/boot/$(KERNEL)
	grub-mkrescue -o $(ISO) iso/

# ============================================
# RUN IN QEMU (virtual machine)
# ============================================
run: $(ISO)
	qemu-system-x86_64 -cdrom $(ISO) -m 32M

# Run without a display window (serial output only)
run-nox: $(ISO)
	qemu-system-x86_64 -cdrom $(ISO) -m 32M -nographic

# ============================================
# CLEAN
# ============================================
clean:
	rm -f $(BOOT_OBJ) $(C_OBJS) $(KERNEL) $(ISO) iso/boot/$(KERNEL)

.PHONY: all run run-nox clean
