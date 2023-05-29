SRC_DIR := src
OUT_DIR := out
LIMINE_SRC_DIR := $(SRC_DIR)/limine
LIMINE_DIR := $(LIMINE_SRC_DIR)/bin
KERNEL_SRC_DIR := $(SRC_DIR)/kernel
KERNEL_OUT_DIR := $(OUT_DIR)/kernel

IMAGE_HDD := image.hdd
IMAGE_ISO := image.iso

# This is the name that our final kernel executable will have.
# Change as needed.
override KERNEL := myos.elf

# Convenience macro to reliably declare overridable command variables.
define DEFAULT_VAR =
    ifeq ($(origin $1),default)
        override $(1) := $(2)
    endif
    ifeq ($(origin $1),undefined)
        override $(1) := $(2)
    endif
endef

# It is highly recommended to use a custom built cross toolchain to build a kernel.
# We are only using "cc" as a placeholder here. It may work by using
# the host system's toolchain, but this is not guaranteed.
$(eval $(call DEFAULT_VAR,CC,cc))

# Same thing for "ld" (the linker).
$(eval $(call DEFAULT_VAR,LD,ld))

# User controllable CFLAGS.
CFLAGS ?= -O2 -Wall -Wextra

# User controllable preprocessor flags. We set none by default.
CPPFLAGS ?=

# User controllable nasm flags.
NASMFLAGS ?= -F dwarf

# User controllable linker flags. We set none by default.
LDFLAGS ?=

# User controllable QEMU flags.
QEMUFLAGS ?= -no-reboot -no-shutdown -m 4G

# Internal C flags that should not be changed by the user.
override CFLAGS +=       \
    -std=c11             \
    -ffreestanding       \
    -fno-stack-protector \
    -fno-stack-check     \
    -fno-lto             \
    -fno-pie             \
    -fno-pic             \
    -m64                 \
    -march=x86-64        \
    -mabi=sysv           \
    -mno-80387           \
    -mno-mmx             \
    -mno-sse             \
    -mno-sse2            \
    -mno-red-zone        \
    -mcmodel=kernel      \
    -MMD                 \
    -I$(KERNEL_SRC_DIR)  \
    -Isrc/limine

# Internal linker flags that should not be changed by the user.
override LDFLAGS +=         \
    -nostdlib               \
    -static                 \
    -m elf_x86_64           \
    -z max-page-size=0x1000 \
    -T $(KERNEL_SRC_DIR)/linker.ld

# Check if the linker supports -no-pie and enable it if it does.
ifeq ($(shell $(LD) --help 2>&1 | grep 'no-pie' >/dev/null 2>&1; echo $$?),0)
    override LDFLAGS += -no-pie
endif

# Internal nasm flags that should not be changed by the user.
override NASMFLAGS += \
    -f elf64

# Use find to glob all *.c, *.S, and *.asm files in the directory and extract the object names.
override CFILES := $(shell find $(KERNEL_SRC_DIR) -type f -name '*.c')
override ASFILES := $(shell find $(KERNEL_SRC_DIR) -type f -name '*.S')
override NASMFILES := $(shell find $(KERNEL_SRC_DIR) -type f -name '*.asm')
override OBJ := $(foreach file,$(CFILES:.c=.o) $(ASFILES:.S=.o) $(NASMFILES:.asm=.o),$(file:$(KERNEL_SRC_DIR)/%=$(KERNEL_OUT_DIR)/%))

# Default target.
.PHONY: all
all: $(KERNEL_OUT_DIR)/$(KERNEL)

# Link rules for the final kernel executable.
$(KERNEL_OUT_DIR)/$(KERNEL): $(OBJ) $(KERNEL_OUT_DIR)
	$(LD) $(OBJ) $(LDFLAGS) -o $@

# Alias for building the kernel.
kernel: $(KERNEL_OUT_DIR)/$(KERNEL)
	@ # Prevent the default cc rule from being run.

# Adding debug flags to the kernel.
kernel_debug: CFLAGS += -DDEBUG -g -save-temps=obj
kernel_debug: NASMFLAGS += -DDEBUG -g
kernel_debug: kernel

# Include header dependencies.
-include $(HEADER_DEPS)

# Compilation rules for *.c files.
$(KERNEL_OUT_DIR)/%.o: $(KERNEL_SRC_DIR)/%.c
	mkdir -p $(shell dirname "$@")
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# Compilation rules for *.S files.
$(KERNEL_OUT_DIR)/%.o: $(KERNEL_SRC_DIR)/%.S
	mkdir -p $(shell dirname "$@")
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# Compilation rules for *.asm (nasm) files.
$(KERNEL_OUT_DIR)/%.o: $(KERNEL_SRC_DIR)/%.asm
	mkdir -p $(shell dirname "$@")
	nasm $(NASMFLAGS) $< -o $@

$(LIMINE_DIR)/limine:
	$(LIMINE_SRC_DIR)/bootstrap
	(cd $(LIMINE_SRC_DIR) && ./configure --enable-all --disable-uefi-aarch64)
	make -j16 -C $(LIMINE_SRC_DIR)

limine: $(LIMINE_DIR)/limine
	@ # Don't run the default rule.

.PHONY: limine-clean
limine-clean:
	make -C $(LIMINE_SRC_DIR) clean

.ONESHELL:
$(OUT_DIR)/$(IMAGE_HDD): $(KERNEL_OUT_DIR)/$(KERNEL) limine
	# Create an empty zeroed out 64MiB image file.
	dd if=/dev/zero bs=1M count=0 seek=64 of=$@
	# Create a GPT partition table.
	parted -s $@ mklabel gpt
	# Create an ESP partition that spans the whole disk.
	parted -s $@ mkpart ESP fat32 2048s 100%
	parted -s $@ set 1 esp on
	# Install the Limine BIOS stages onto the image.
	$(LIMINE_DIR)/limine bios-install $@
	# Mount the loopback device.
	USED_LOOPBACK=$$(sudo losetup -Pf --show $@)
	# Format the ESP partition as FAT32.
	sudo mkfs.fat -F 32 $${USED_LOOPBACK}p1
	# Mount the partition itself.
	mkdir -p $(OUT_DIR)/img_mount
	sudo mount $${USED_LOOPBACK}p1 $(OUT_DIR)/img_mount
	# Copy the relevant files over.
	sudo mkdir -p $(OUT_DIR)/img_mount/EFI/BOOT
	sudo cp -v \
		$(KERNEL_OUT_DIR)/$(KERNEL) \
		$(SRC_DIR)/limine.cfg \
		$(LIMINE_DIR)/limine.sys \
		$(OUT_DIR)/img_mount/
	sudo cp -v \
		$(LIMINE_DIR)/BOOTX64.EFI \
		$(OUT_DIR)/img_mount/EFI/BOOT/
	# Sync system cache and unmount partition and loopback device.
	sync
	sudo umount $(OUT_DIR)/img_mount
	sudo losetup -d $${USED_LOOPBACK}

$(OUT_DIR)/$(IMAGE_ISO): $(KERNEL_OUT_DIR)/$(KERNEL) limine
	mkdir -p $(OUT_DIR)/iso_root
	cp -v $(KERNEL_OUT_DIR)/$(KERNEL) \
		$(SRC_DIR)/limine.cfg \
		$(LIMINE_DIR)/limine.sys \
		$(LIMINE_DIR)/limine-cd.bin \
		$(LIMINE_DIR)/limine-cd-efi.bin \
		$(OUT_DIR)/iso_root
	# Create the bootable ISO.
	xorriso -as mkisofs \
		-b limine-cd.bin \
		-no-emul-boot \
		-boot-load-size 4 \
		-boot-info-table \
		--efi-boot limine-cd-efi.bin \
		-efi-boot-part \
		--efi-boot-image \
		--protective-msdos-label \
		$(OUT_DIR)/iso_root \
		-o $@
	# Install Limine stage 1 and 2 for legacy BIOS boot.
	$(LIMINE_DIR)/limine bios-install $@

.PHONY:
run_hdd: $(OUT_DIR)/$(IMAGE_HDD)
	qemu-system-x86_64 $(QEMUFLAGS) $<

.PHONY:
run_iso: $(OUT_DIR)/$(IMAGE_ISO)
	qemu-system-x86_64 $(QEMUFLAGS) $<

# Remove object files and the final executable.
.PHONY: clean
clean:
	sudo rm -rf $(OUT_DIR)
