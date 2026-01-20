CC = i686-elf-gcc
AS = nasm
AR = i686-elf-ar

KERNEL_UAPI = ../../../kernel/include
SRCDIR = src
BUILDDIR = build
INCDIR = include

CFLAGS = -I$(INCDIR) -I$(KERNEL_UAPI) \
         -m32 -ffreestanding -nostdlib -fno-builtin \
         -Wall -Wextra -O0 -std=gnu17 

ASFLAGS = -felf32

C_SOURCES = $(shell find $(SRCDIR) -name '*.c')
ASM_SOURCES = $(shell find $(SRCDIR) -name '*.asm')

C_OBJECTS = $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(C_SOURCES))
ASM_OBJECTS = $(patsubst $(SRCDIR)/%.asm,$(BUILDDIR)/%.o,$(filter-out $(SRCDIR)/crt0.asm,$(ASM_SOURCES)))

CRT0 = $(BUILDDIR)/crt0.o

all: libc.a $(CRT0)

libc.a: $(C_OBJECTS) $(ASM_OBJECTS)
	@echo "AR   => $@"
	@$(AR) rcs $@ $^

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	@echo "CC   => $<"
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/%.o: $(SRCDIR)/%.asm
	@echo "AS   => $<"
	@mkdir -p $(dir $@)
	@$(AS) $(ASFLAGS) $< -o $@

clean:
	@echo "CLEAN libc"
	@rm -rf $(BUILDDIR) libc.a

re: clean all

.PHONY: all clean re
