
# This will need to be set depending on where you've placed the u-boot binary
UBOOT_BIN ?= ../u-boot-arm64.bin

CFLAGS += \
	  -mno-outline-atomics \
	  -mstrict-align \
	  -Werror=pointer-sign \
	  -fomit-frame-pointer \
	  -Werror-implicit-function-declaration

LDFLAGS += --fatal-warnings

QEMU_FLASH = $(SETUPS_DIR)/$(ARCH)/flash.img

QEMU := qemu-system-aarch64
QEMU_DEPS += uImage $(QEMU_FLASH) $(UBOOT_BIN)

QEMU_MACHINE_FLAGS += virt,virtualization=on#,secure=on

ifdef NAUT_CONFIG_GIC_VERSION_2
	QEMU_MACHINE_FLAGS := $(QEMU_MACHINE_FLAGS),gic-version=2
endif

ifdef NAUT_CONFIG_GIC_VERSION_3
	QEMU_MACHINE_FLAGS := $(QEMU_MACHINE_FLAGS),gic-version=3,its=on
endif

QEMU_FLAGS += -kernel uImage -bios $(UBOOT_BIN) \
			  --cpu cortex-a72 \
			  --machine $(QEMU_MACHINE_FLAGS) \
			  -drive if=pflash,format=raw,index=1,file=$(QEMU_FLASH)

