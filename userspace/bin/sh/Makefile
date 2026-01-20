CC = i686-elf-gcc

LIBC_DIR = ../../lib/libc
KERNEL_INCLUDE = ../../../kernel/include

CFLAGS = -m32 -nostdlib -ffreestanding -O2 -Wall -std=gnu17 \
         -I$(LIBC_DIR)/include -I$(KERNEL_INCLUDE)

LDFLAGS = -m32 -nostdlib 

all: sh

sh: $(LIBC_DIR)/build/crt0.o shell.o $(LIBC_DIR)/libc.a
	@echo "LD   => $@"
	@$(CC) $(LDFLAGS) -o $@ $^ -lgcc

shell.o: shell.c
	@echo "CC   => $<"
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o sh

.PHONY: all clean
