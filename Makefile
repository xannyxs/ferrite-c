KERNEL_DIR = kernel
KERNEL_ELF = $(KERNEL_DIR)/kernel.elf

ROOT_IMG = root.img
TEST_IMG = test.img

ISO_NAME = ferrite.iso
ISO_DIR = isodir

QEMUFLAGS = -serial stdio -m 16 -cpu 486 \
    -drive file=$(ROOT_IMG),format=raw,if=ide,index=0 \
    -drive file=$(TEST_IMG),format=raw,if=ide,index=1

all: kernel

kernel:
	@echo "=== Building Kernel ==="
	@$(MAKE) -C $(KERNEL_DIR)

images: $(ROOT_IMG) $(TEST_IMG)

$(ROOT_IMG):
	@echo "Creating root disk image (10MB)..."
	@qemu-img create -f raw $(ROOT_IMG) 10M
	@mkfs.ext2 -F $(ROOT_IMG)

$(TEST_IMG):
	@echo "Creating test disk image (50MB)..."
	@qemu-img create -f raw $(TEST_IMG) 50M
	@mkfs.ext2 -F $(TEST_IMG)

iso: kernel
	@echo "=== Creating ISO ==="
	@mkdir -p $(ISO_DIR)/boot/grub
	@cp $(KERNEL_ELF) $(ISO_DIR)/boot/kernel.elf
	@cp grub.cfg $(ISO_DIR)/boot/grub/grub.cfg
	@grub-mkrescue -o $(ISO_NAME) $(ISO_DIR) 2>/dev/null
	@echo "ISO created: $(ISO_NAME)"
	
run: iso images
	@echo "=== Running in QEMU ==="
	qemu-system-i386 -cdrom $(ISO_NAME) $(QEMUFLAGS)

debug: QEMUFLAGS += -s -S
debug: iso images
	@echo "=== Debug Mode (waiting for GDB on :1234) ==="
	qemu-system-i386 -cdrom $(ISO_NAME) $(QEMUFLAGS)

debug_bochs: iso
	@echo "=== Running in Bochs ==="
	bochs -f .bochsrc -q

test: QEMUFLAGS += -device isa-debug-exit,iobase=0xf4,iosize=0x04 -display none
test: iso images
	@echo "=== Test Mode ==="
	qemu-system-i386 -cdrom $(ISO_NAME) $(QEMUFLAGS)

clean:
	@echo "=== Cleaning Build Artifacts ==="
	@$(MAKE) -C $(KERNEL_DIR) clean
	@rm -rf $(ISO_DIR) $(ISO_NAME)

fclean: clean
	@echo "=== Full Clean ==="
	@rm -f $(ROOT_IMG) $(TEST_IMG)

re: fclean all

lint:
	@echo "=== Running Linters ==="
	@$(MAKE) -C $(KERNEL_DIR) lint

.PHONY: all kernel iso images run debug debug_bochs test clean fclean re
