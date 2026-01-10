CC = i686-elf-gcc
LD = i686-elf-gcc 
AS = nasm 
ZIG = zig

NAME = kernel.elf
ARCH = x86

ZIG_DIR = ./drivers
INCDIR = ./include
SDIR = ./src
ODIR = ./build

CFLAGS = -I$(SDIR) -I$(INCDIR) -I$(SDIR)/arch/$(ARCH) \
         -m32 -ffreestanding -ggdb3 -O2 \
         -Wall -Wextra -Werror -pedantic -std=c17 \
         -fno-stack-protector -march=i386 -nostdlib \
         -D__DEBUG -D__is_libk -D__print_serial -D__bochs

ASFLAGS = -felf32

LDFLAGS = -T $(SDIR)/arch/x86/x86.ld \
          -ffreestanding -nostdlib -march=i386

ZIG_TARGET = x86-freestanding
ZIG_CPU = i686
ZIG_FLAGS = -mcpu=$(ZIG_CPU) -fno-stack-protector -O ReleaseSmall

C_SOURCES = $(shell find $(SDIR) -type f -name '*.c')
ASM_SOURCES = $(shell find $(SDIR) -type f -name '*.asm')
ZIG_SOURCES = $(shell find $(ZIG_DIR)/src -type f -name '*.zig')

C_OBJECTS = $(patsubst $(SDIR)/%.c,$(ODIR)/%.o,$(C_SOURCES))
ASM_OBJECTS = $(patsubst $(SDIR)/%.asm,$(ODIR)/%.o,$(ASM_SOURCES))
ZIG_OBJECTS = $(patsubst $(ZIG_DIR)/src/%.zig,$(ODIR)/zig/%.o,$(ZIG_SOURCES))

all: $(NAME)

$(NAME): $(ASM_OBJECTS) $(C_OBJECTS) $(ZIG_OBJECTS)
	@echo "LD   => $@"
	@$(LD) -o $@ $^ $(LDFLAGS) -lgcc

$(ODIR)/%.o: $(SDIR)/%.c
	@echo "CC   => $<"
	@mkdir -p $(dir $@)
	@$(CC) -c $< -o $@ $(CFLAGS)

$(ODIR)/%.o: $(SDIR)/%.asm
	@echo "AS   => $<"
	@mkdir -p $(dir $@)
	@$(AS) $(ASFLAGS) $< -o $@

$(ODIR)/zig/%.o: $(ZIG_DIR)/src/%.zig
	@echo "ZIG  => $<"
	@mkdir -p $(dir $@)
	@$(ZIG) build-obj \
		-target $(ZIG_TARGET) \
		$(ZIG_FLAGS) \
		-I$(INCDIR) \
		-I$(SDIR) \
		$< --name $(basename $(notdir $@))
	@mv $(basename $(notdir $@)).o $@

clean:
	@echo "CLEAN kernel"
	@rm -rf $(ODIR) $(NAME)

re: clean all

lint:
	@echo "Running clang-tidy on kernel..."
	@find $(SDIR) -name '*.c' | while read file; do \
		echo "Checking $$file..."; \
		clang-tidy "$$file"; \
	done
	@echo "Lint complete!"

.PHONY: all clean re lint
