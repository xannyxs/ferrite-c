CC = i686-elf-gcc
LD = i686-elf-gcc  
AS = nasm 

NAME = ferrite-c.bin

SDIR = ./src
ODIR = ./build

CFLAGS = -I$(SDIR) -m32 -ffreestanding -O2 -Wall -Wextra -Werror \
         -fno-stack-protector -D__is_libk
ASFLAGS = -felf32
LDFLAGS = -T $(SDIR)/arch/x86/x86.ld -ffreestanding -nostdlib -lgcc

C_SOURCES = $(shell find $(SDIR) -type f -name '*.c')
ASM_SOURCES = $(shell find $(SDIR) -type f -name '*.asm')

C_OBJECTS = $(patsubst $(SDIR)/%.c,$(ODIR)/%.o,$(C_SOURCES))
ASM_OBJECTS = $(patsubst $(SDIR)/%.asm,$(ODIR)/%.o,$(ASM_SOURCES))

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

run: all
	qemu-system-i386 -kernel $(NAME)

debug: all
	qemu-system-i386 -kernel $(NAME) -s -S

clean:
	@echo "CLEAN"
	@rm -rf $(ODIR) $(NAME)

re:
	@$(MAKE) clean
	@$(MAKE) all

.PHONY: all run clean re
