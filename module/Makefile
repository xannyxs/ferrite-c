CC = i686-elf-gcc

KERNEL_INCLUDE = ../../../kernel/include
CFLAGS = -m32 -nostdlib -ffreestanding -O2 -Wall -fno-pic \
         -fno-asynchronous-unwind-tables -fno-stack-protector \
         -I$(LIBC_DIR)/include -I$(KERNEL_INCLUDE)

SRCS = src/keyboard_module.c src/basic_module.c src/timer_module.c
OBJS = $(SRCS:.c=.o)

all: $(OBJS)

%.o: %.c
	@echo "CC   => $<"
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	@rm -f $(OBJS)

.PHONY: all clean
