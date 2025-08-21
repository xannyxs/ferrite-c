CC = i686-elf-gcc
LD = i686-elf-gcc # Change to "ld -m elf_i386"?
AS = nasm 

NAME = ferrite-c.elf

SDIR = ./src
ODIR = ./build

ASFLAGS = -felf32
LDFLAGS = -T $(SDIR)/arch/x86/x86.ld -ffreestanding -nostdlib -lgcc -march=i386
CFLAGS = -I$(SDIR) -m32 -ffreestanding -g -O2 -Wall -Wextra -Werror \
				 -fno-stack-protector -D__IS_LIBK -D__PRINT_SERIAL \
				 -pedantic -std=c99 -march=i386

C_SOURCES = $(shell find $(SDIR) -type f -name '*.c')
ASM_SOURCES = $(shell find $(SDIR) -type f -name '*.asm')

C_OBJECTS = $(patsubst $(SDIR)/%.c,$(ODIR)/%.o,$(C_SOURCES))
ASM_OBJECTS = $(patsubst $(SDIR)/%.asm,$(ODIR)/%.o,$(ASM_SOURCES))

QEMUFLAGS = -serial stdio -m 8

all: $(NAME)

$(NAME): $(ASM_OBJECTS) $(C_OBJECTS)
	@echo "LD   => $@"
	@$(LD) -o $@ $^ $(LDFLAGS)

$(ODIR)/%.o: $(SDIR)/%.c
	@echo "CC   => $<"
	@mkdir -p $(dir $@)
	@$(CC) -c $< -o $@ $(CFLAGS)

$(ODIR)/%.o: $(SDIR)/%.asm
	@echo "AS   => $<"
	@mkdir -p $(dir $@)
	@$(AS) $(ASFLAGS) $< -o $@

iso: all
	mkdir -p isodir/boot/grub
	cp $(NAME) isodir/boot/$(NAME)
	cp grub.cfg isodir/boot/grub/grub.cfg
	grub-mkrescue -o kernel.iso isodir

run: iso 
	qemu-system-i386 -cdrom kernel.iso $(QEMUFLAGS)

run_bochs: CFLAGS += -D__BOCHS
run_bochs: iso 
	bochs -f .bochsrc -q

debug_bochs: CFLAGS += -D__BOCHS -D__DEBUG
debug_bochs: iso 
	bochs -f .bochsrc -q

debug: QEMUFLAGS += -s -S
debug: run 

test: QEMUFLAGS += -device isa-debug-exit,iobase=0xf4,iosize=0x04 -display none
test: run

clean:
	@echo "CLEAN"
	@rm -rf $(ODIR) $(NAME)

fclean:
	@echo "FCLEAN"
	@rm -rf $(ODIR) $(NAME) isodir kernel.iso

re:
	@$(MAKE) fclean
	@$(MAKE) all

.PHONY: all run test clean fclean re debug iso
