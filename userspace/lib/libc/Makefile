CC = i686-elf-gcc
AS = nasm
AR = i686-elf-ar
CFLAGS = -m32 -nostdlib -ffreestanding -O0 -Wall -Wextra -lgcc
ASFLAGS = -f elf32

BUILD_DIR = build
OBJS = $(BUILD_DIR)/syscalls.o $(BUILD_DIR)/string.o $(BUILD_DIR)/printf.o

all: libc.a $(BUILD_DIR)/crt0.o

libc.a: $(OBJS)
	$(AR) rcs $@ $^

$(BUILD_DIR)/crt0.o: crt0.asm | $(BUILD_DIR)
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/syscalls.o: syscalls.asm | $(BUILD_DIR)
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/string.o: string.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/printf.o: stdio/printf.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR) libc.a

.PHONY: all clean
