CC = i686-elf-gcc
LD = i686-elf-gcc 
AS = nasm 

NAME = kernel.elf
ARCH = x86

DRIVER_DIR = ./drivers
INCDIR = ./include
SDIR = ./src
ODIR = ./build

CFLAGS = -I$(SDIR) -I$(INCDIR) -I$(SDIR)/arch/$(ARCH) \
         -m32 -ffreestanding -ggdb3 -O2 \
         -Wall -Wextra -Werror -Wstrict-prototypes \
         -Wformat=2 -std=gnu17 \
         -fno-stack-protector -march=i386 -nostdlib \
         -D__DEBUG -D__is_libk -D__print_serial -D__bochs

         # -Wvla

ASFLAGS = -felf32

LDFLAGS = -T $(SDIR)/arch/x86/x86.ld \
          -ffreestanding -nostdlib -march=i386

C_SOURCES = $(shell find $(SDIR) -type f -name '*.c')
DRIVER_C_SOURCES = $(shell find $(DRIVER_DIR) -type f -name '*.c')
ASM_SOURCES = $(shell find $(SDIR) -type f -name '*.asm')

C_OBJECTS = $(patsubst $(SDIR)/%.c,$(ODIR)/%.o,$(C_SOURCES))
DRIVER_C_OBJECTS = $(patsubst $(DRIVER_DIR)/%.c,$(ODIR)/drivers/%.o,$(DRIVER_C_SOURCES))
ASM_OBJECTS = $(patsubst $(SDIR)/%.asm,$(ODIR)/%.o,$(ASM_SOURCES))

all: $(NAME)

$(NAME): $(ASM_OBJECTS) $(C_OBJECTS) $(DRIVER_C_OBJECTS) 
	@echo "LD   => $@"
	@$(LD) -o $@ $^ $(LDFLAGS) -lgcc

$(ODIR)/%.o: $(SDIR)/%.c
	@echo "CC   => $<"
	@mkdir -p $(dir $@)
	@$(CC) -c $< -o $@ $(CFLAGS)

$(ODIR)/drivers/%.o: $(DRIVER_DIR)/%.c
	@echo "CC   => $<"
	@mkdir -p $(dir $@)
	@$(CC) -c $< -o $@ $(CFLAGS)

$(ODIR)/%.o: $(SDIR)/%.asm
	@echo "AS   => $<"
	@mkdir -p $(dir $@)
	@$(AS) $(ASFLAGS) $< -o $@

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
	@find $(DRIVER_DIR) -name '*.c' | while read file; do \
		echo "Checking $$file..."; \
		clang-tidy "$$file"; \
	done
	@echo "Lint complete!"

.PHONY: all clean re lint
