CC = i686-elf-gcc

LIBC_DIR = ../../lib/libc
KERNEL_INCLUDE = ../../../kernel/include

CFLAGS = -m32 -nostdlib -ffreestanding -O0 -Wall \
         -I$(LIBC_DIR)/include -I$(KERNEL_INCLUDE)

LDFLAGS = -m32 -nostdlib 

all: insmod 

insmod: $(LIBC_DIR)/build/crt0.o insmod.o $(LIBC_DIR)/libc.a
	@echo "LD   => $@"
	@$(CC) $(LDFLAGS) -o $@ $^ -lgcc

insmod.o: insmod.c
	@echo "CC   => $<"
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o insmod

.PHONY: all clean
