
UBOOT_BIN = ../u-boot.bin

uImage: $(NAUT_BIN)
	$(call quiet_cmd,OBJCOPY,Image)
	$(Q)$(OBJCOPY) -O binary $< Image
	$(call quiet_cmd,MKIMAGE,$@)
	$(Q)mkimage -A arm64 -O linux -T kernel -C none \
		-a $(NAUT_CONFIG_KERNEL_LINK_ADDR) -e $(NAUT_CONFIG_KERNEL_LINK_ADDR) -n "Nautilus" \
		-d Image uImage

QEMU_FLASH = $(SETUPS_DIR)/$(ARCH)/flash.img

qemu-flash: $(QEMU_FLASH)
$(QEMU_FLASH):
	qemu-img create -f raw $(QEMU_FLASH) 64M

QEMU_GDB_FLAGS := #-gdb tcp::5060 -S
QEMU_DEVICES := -display none
QEMU_DEVICES += -drive if=pflash,format=raw,index=1,file=$(QEMU_FLASH)
QEMU_DEVICES += #-device virtio-gpu-pci #-netdev socket,id=net0,listen=localhost:4756 -device e1000e,netdev=net0,mac=00:11:22:33:44:55

QEMU_MACHINE_FLAGS = virt,virtualization=on#,secure=on

ifdef NAUT_CONFIG_GIC_VERSION_2
	QEMU_MACHINE_FLAGS := $(QEMU_MACHINE_FLAGS),gic-version=2
endif

ifdef NAUT_CONFIG_GIC_VERSION_3
	QEMU_MACHINE_FLAGS := $(QEMU_MACHINE_FLAGS),gic-version=3,its=on
endif

qemu: uImage
	qemu-system-aarch64 \
		-smp cpus=4 \
		--cpu cortex-a72 \
		-serial stdio \
		--machine $(QEMU_MACHINE_FLAGS) \
		-bios $(UBOOT_BIN) \
		-kernel uImage \
		$(QEMU_GDB_FLAGS) \
		$(QEMU_DEVICES) \
		-numa node,cpus=0-1,memdev=m0 \
		-numa node,cpus=2-3,memdev=m1 \
		-object memory-backend-ram,id=m0,size=2G \
		-object memory-backend-ram,id=m1,size=2G \
		-m 4G

qemu-raw: $(BIN_NAME)
	qemu-system-aarch64 \
		-smp cpus=4 \
		--cpu cortex-a53 \
		-serial stdio \
		--machine $(QEMU_MACHINE_FLAGS) \
		-kernel $(BIN_NAME) \
		$(QEMU_GDB_FLAGS) \
		$(QEMU_DEVICES) \
		-m 512M

qemu-up: uImage
	qemu-system-aarch64 \
		--cpu cortex-a72 \
		-serial stdio \
		--machine $(QEMU_MACHINE_FLAGS) \
		-bios $(UBOOT_BIN) \
		-kernel uImage \
		$(QEMU_GDB_FLAGS) \
		$(QEMU_DEVICES) \
		-m 2G

clean-arm64:
	$(Q)rm -f Image
	$(Q)rm -f uImage

DEFAULT_RULES += uImage
CLEAN_RULES += clean-arm64

.PHONY: qemu-flash
