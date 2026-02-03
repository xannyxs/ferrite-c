KERNEL_DIR = kernel
KERNEL_ELF = $(KERNEL_DIR)/kernel.elf

USERSPACE_DIR = userspace
MODULE_DIR = module
SYSROOT_DIR = sysroot

ROOT_IMG = root.img
TEST_IMG = test.img

ISO_NAME = ferrite.iso
ISO_DIR = isodir

QEMUFLAGS = -serial stdio -m 16 -cpu 486 \
    -drive file=$(ROOT_IMG),format=raw,if=ide,index=0 \
    -drive file=$(TEST_IMG),format=raw,if=ide,index=1

all: kernel userspace

kernel:
	@echo "=== Building Kernel ==="
	@$(MAKE) -C $(KERNEL_DIR)

userspace:
	@echo "=== Building Userspace ==="
	@$(MAKE) -C $(USERSPACE_DIR)

modules:
	@echo "=== Building Modules ==="
	@$(MAKE) -C $(MODULE_DIR)

install: kernel userspace modules
	@echo "=== Installing to sysroot ==="
	@mkdir -p $(SYSROOT_DIR)/{bin,boot,module}
	@cp $(KERNEL_ELF) $(SYSROOT_DIR)/boot/kernel.elf
	@cp $(USERSPACE_DIR)/bin/sh/sh $(SYSROOT_DIR)/bin/
	@cp $(USERSPACE_DIR)/bin/hello/hello $(SYSROOT_DIR)/bin/
	@cp $(USERSPACE_DIR)/bin/ls/ls $(SYSROOT_DIR)/bin/
	@cp $(USERSPACE_DIR)/bin/touch/touch $(SYSROOT_DIR)/bin/
	@cp $(USERSPACE_DIR)/bin/cat/cat $(SYSROOT_DIR)/bin/
	@cp $(USERSPACE_DIR)/bin/rmdir/rmdir $(SYSROOT_DIR)/bin/
	@cp $(USERSPACE_DIR)/bin/mkdir/mkdir $(SYSROOT_DIR)/bin/
	@cp $(USERSPACE_DIR)/bin/rm/rm $(SYSROOT_DIR)/bin/
	@cp $(USERSPACE_DIR)/bin/time/time $(SYSROOT_DIR)/bin/
	@cp $(USERSPACE_DIR)/bin/rmmod/rmmod $(SYSROOT_DIR)/bin/
	@cp $(USERSPACE_DIR)/bin/insmod/insmod $(SYSROOT_DIR)/bin/
	@cp $(USERSPACE_DIR)/bin/mount/mount $(SYSROOT_DIR)/bin/
	@cp $(USERSPACE_DIR)/bin/test/test $(SYSROOT_DIR)/bin/
	@cp $(MODULE_DIR)/src/basic_module.o $(SYSROOT_DIR)/module/
	@cp $(MODULE_DIR)/src/keyboard_module.o $(SYSROOT_DIR)/module/
	@cp $(MODULE_DIR)/src/timer_module.o $(SYSROOT_DIR)/module/
	@echo "Sysroot populated"

images: $(ROOT_IMG) $(TEST_IMG)

$(ROOT_IMG): install
	@echo "Creating root disk image (10MB)..."
	@qemu-img create -f raw $(ROOT_IMG) 10M
	@mkfs.ext2 -F $(ROOT_IMG)
	@echo "Copying files to root image..."
	@mkdir -p mnt
	@sudo mount -o loop $(ROOT_IMG) mnt
	@sudo mkdir -p mnt/{bin,sbin,dev,module}
	@sudo cp $(SYSROOT_DIR)/bin/* mnt/bin/ 2>/dev/null || true
	@sudo cp $(SYSROOT_DIR)/module/* mnt/module/ 2>/dev/null || true
	@sudo umount mnt
	@rmdir mnt
	@echo "Root image ready"

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

clean:
	@echo "=== Cleaning Build Artifacts ==="
	@$(MAKE) -C $(KERNEL_DIR) clean
	@$(MAKE) -C $(USERSPACE_DIR) clean
	@$(MAKE) -C $(MODULE_DIR) clean
	@rm -rf $(ISO_DIR) $(ISO_NAME)
	@rm -rf $(SYSROOT_DIR)

fclean: clean
	@echo "=== Full Clean ==="
	@rm -f $(ROOT_IMG) $(TEST_IMG)

re: fclean all

lint:
	@echo "=== Running Linters ==="
	@$(MAKE) -C $(KERNEL_DIR) lint

.PHONY: all kernel userspace install images iso run debug debug_bochs test clean fclean re lint
