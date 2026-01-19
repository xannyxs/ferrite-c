CC = i686-elf-gcc
LIBC_DIR = ../../lib/libc

CFLAGS = -m32 -nostdlib -ffreestanding -O2 -Wall -I$(LIBC_DIR) 
LDFLAGS = -m32 -nostdlib 

all: sh

sh: shell.o $(LIBC_DIR)/build/crt0.o $(LIBC_DIR)/libc.a
	$(CC) $(LDFLAGS) -o $@ $(LIBC_DIR)/build/crt0.o shell.o $(LIBC_DIR)/libc.a -lgcc

shell.o: shell.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o sh

.PHONY: all clean
