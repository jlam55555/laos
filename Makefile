SRC_DIR := src
OUT_DIR := out
LIMINE_SRC_DIR := $(SRC_DIR)/limine
LIMINE_DIR := $(LIMINE_SRC_DIR)/bin
KERNEL_SRC_DIR := $(SRC_DIR)/kernel

# Purposely not :=
KERNEL_OUT_DIR = $(OUT_DIR)/kernel

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
QEMUFLAGS ?= -m 4G

# QEMU allows commas in comma-separated argument lists if the commas are
# duplicated. See QEMU_FLAGS and run_{hdd,iso} for usages.
COMMA := ,
QEMU_OUT_DIR = $(subst $(COMMA),$(COMMA)$(COMMA),$(OUT_DIR))

# Adding tests if necessary. Specify using `make RUNTEST=<test_selection> ...`
# Creates a new build variant to avoid inconsistent builds. Use the special
# test selection "RUNTEST=all" to run all tests. Tests are written to the
# test.out file in the output directory.
ifneq ($(RUNTEST),)
    ifeq ($(RUNTEST),all)
        override CFLAGS += -DRUNTEST='' -DSERIAL
    else
        override CFLAGS += -DRUNTEST=$(RUNTEST) -DSERIAL
    endif
    override OUT_DIR := $(OUT_DIR).test_$(RUNTEST)

    # tee-like functionality. See https://superuser.com/a/1412150
    override QEMUFLAGS += -display none \
        -chardev stdio,id=char0,logfile=$(QEMU_OUT_DIR)/test.out,signal=off \
        -serial chardev:char0
else ifneq($(SERIAL),)
    # This doesn't create a different variant. Should it?
    override CFLAGS += -DSERIAL
    override QEMUFLAGS += -serial stdio
endif

# Debug mode. Specify using `make DEBUG=1 ...`
# Make sure to `make clean` if switching between debug and non-debug modes to
# prevent an inconsistent build.
# Also creates a new build variant.
ifneq ($(DEBUG),)
    override CFLAGS += -DDEBUG -g -save-temps=obj
    override NASMFLAGS += -DDEBUG -g
    override OUT_DIR := $(OUT_DIR).debug
    ifeq ($(DEBUG),i)
        override QEMUFLAGS += -s -S -no-reboot -no-shutdown
    endif
endif

# See final output directory. https://stackoverflow.com/a/16489377/2397327
$(info $$OUT_DIR is [${OUT_DIR}])

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
	(cd $(LIMINE_SRC_DIR) && ./configure --enable-all --disable-uefi-aarch64 --disable-uefi-riscv64)
	make -j16 -C $(LIMINE_SRC_DIR)

limine: $(LIMINE_DIR)/limine
	@ # Don't run the default rule.

.PHONY: limine-clean
limine-clean:
	make -C $(LIMINE_SRC_DIR) clean

.ONESHELL:
$(OUT_DIR)/$(IMAGE_HDD): $(KERNEL_OUT_DIR)/$(KERNEL) limine
	# Create an empty zeroed-out 64MiB image file.
	dd if=/dev/zero bs=1M count=0 seek=64 of=$@

	# Create a GPT partition table.
	sgdisk $@ -n 1:2048 -t 1:ef00

	# Install the Limine BIOS stages onto the image.
	$(LIMINE_DIR)/limine bios-install $@

	# Format the image as fat32.
	mformat -i $@@@1M

	# Make /EFI and /EFI/BOOT an MSDOS subdirectory.
	mmd -i $@@@1M ::/EFI ::/EFI/BOOT

	# Copy over the relevant files
	mcopy -i $@@@1M \
		$(KERNEL_OUT_DIR)/$(KERNEL) \
		$(SRC_DIR)/limine.cfg \
		$(LIMINE_DIR)/limine-bios.sys ::/
	mcopy -i $@@@1M $(LIMINE_DIR)/BOOTX64.EFI ::/EFI/BOOT
	mcopy -i $@@@1M $(LIMINE_DIR)/BOOTIA32.EFI ::/EFI/BOOT

$(OUT_DIR)/$(IMAGE_ISO): $(KERNEL_OUT_DIR)/$(KERNEL) limine
	mkdir -p $(OUT_DIR)/iso_root
	cp -v $(KERNEL_OUT_DIR)/$(KERNEL) \
		$(SRC_DIR)/limine.cfg \
		$(LIMINE_DIR)/limine-bios.sys \
		$(LIMINE_DIR)/limine-bios-cd.bin \
		$(LIMINE_DIR)/limine-uefi-cd.bin \
		$(OUT_DIR)/iso_root
	# Create the bootable ISO.
	xorriso -as mkisofs \
		-b limine-bios-cd.bin \
		-no-emul-boot \
		-boot-load-size 4 \
		-boot-info-table \
		--efi-boot limine-uefi-cd.bin \
		-efi-boot-part \
		--efi-boot-image \
		--protective-msdos-label \
		$(OUT_DIR)/iso_root \
		-o $@
	# Install Limine stage 1 and 2 for legacy BIOS boot.
	$(LIMINE_DIR)/limine bios-install $@

.PHONY:
run_hdd: $(OUT_DIR)/$(IMAGE_HDD)
	qemu-system-x86_64 $(QEMUFLAGS) \
	    -drive file=$(QEMU_OUT_DIR)/$(IMAGE_HDD),media=disk,format=raw

.PHONY:
run_iso: $(OUT_DIR)/$(IMAGE_ISO)
	qemu-system-x86_64 $(QEMUFLAGS) \
	    -drive file=$(QEMU_OUT_DIR)/$(IMAGE_ISO),media=disk,format=raw

# Remove object files and the final executable.
.PHONY: clean
clean:
	sudo rm -rf $(OUT_DIR)
