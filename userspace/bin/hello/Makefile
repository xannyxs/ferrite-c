CC = i686-elf-gcc

LIBC_DIR = ../../lib/libc
KERNEL_INCLUDE = ../../../kernel/include

CFLAGS = -m32 -nostdlib -ffreestanding -O2 -Wall \
         -I$(LIBC_DIR)/include -I$(KERNEL_INCLUDE)

LDFLAGS = -m32 -nostdlib

all: hello

hello: $(LIBC_DIR)/build/crt0.o hello.o $(LIBC_DIR)/libc.a
	@echo "LD   => $@"
	@$(CC) $(LDFLAGS) -o $@ $^ -lgcc

hello.o: hello.c
	@echo "CC   => $<"
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o hello

.PHONY: all clean
