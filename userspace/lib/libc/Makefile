CC = i686-elf-gcc
AS = nasm
AR = i686-elf-ar

CFLAGS = -m32 -nostdlib -ffreestanding -O2 -Wall
ASFLAGS = -f elf32

OBJS = crt0.o syscalls.o string.o

all: libc.a crt0.o

libc.a: syscalls.o string.o
	$(AR) rcs $@ $^

%.o: %.asm
	$(AS) $(ASFLAGS) $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o libc.a

.PHONY: all clean
