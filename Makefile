
export

default:
	@:

ROOT_DIR=$(shell pwd)
ABS_ROOT_DIR=$(ROOT_DIR)

SOURCE_DIR:=$(ROOT_DIR)/src
LIBRARY_DIR:=$(ROOT_DIR)/lib
INCLUDE_DIR:=$(ROOT_DIR)/include
OUTPUT_DIR:=$(ROOT_DIR)
SCRIPTS_DIR:=$(ROOT_DIR)/scripts
SETUPS_DIR:=$(ROOT_DIR)/setups
LINK_DIR:=$(ROOT_DIR)/link

# Config dependency
PYTHON=python3

include $(SCRIPTS_DIR)/include.mk
include $(SCRIPTS_DIR)/kconfig.mk

# This checks to make sure that the .config file exists and has been loaded
ifndef NAUT_CONFIG_CONFIG

# default to x86 if no config exists yet
ARCH ?= x64

default: no_config_message
no_config_message:
	echo "No .config file found!"
	echo "Run 'make defconfig' or 'make menuconfig' to configure the kernel"

else

# Architecture Specific Build Configurations
ifdef NAUT_CONFIG_ARCH_X86
ARCH ?= x64
endif
ifdef NAUT_CONFIG_ARCH_RISCV
ARCH ?= riscv
endif
ifdef NAUT_CONFIG_ARCH_ARM64
ARCH ?= arm64
endif

ARCH_SCRIPTS_DIR:=$(SCRIPTS_DIR)/arch/$(ARCH)
-include $(ARCH_SCRIPTS_DIR)/config.mk

ifdef NAUT_CONFIG_DEBUG_INFO
CFLAGS		+= -g
endif

# Toolchain Specific Build Configurations
ifdef NAUT_CONFIG_USE_CLANG
TOOLCHAIN ?= clang
endif
ifdef NAUT_CONFIG_USE_GCC
TOOLCHAIN ?= gcc
endif
ifdef NAUT_CONFIG_USE_WLLVM
TOOLCHAIN ?= wllvm
endif

TOOLCHAIN_SCRIPTS_DIR:=$(SCRIPTS_DIR)/toolchain/$(TOOLCHAIN)
-include $(TOOLCHAIN_SCRIPTS_DIR)/config.mk

# General Build Configuration

LD_SCRIPT_SRC ?= $(LINK_DIR)/nautilus.ld.$(ARCH)
LD_SCRIPT = $(LINK_DIR)/link.ld

COMMON_FLAGS += -fno-omit-frame-pointer \
			   -ffreestanding \
			   -fno-stack-protector \
			   -fno-strict-aliasing \
                           -fno-strict-overflow \

COMMON_FLAGS += -Wno-int-conversion

NAUT_INCLUDE += -D__NAUTILUS__ -I$(INCLUDE_DIR) \
		     -include $(AUTOCONF)

NAUT_BIN_NAME := nautilus.bin
NAUT_ASM_NAME := nautilus.asm

NAUT_BIN := $(OUTPUT_DIR)/$(NAUT_BIN_NAME)
NAUT_ASM := $(OUTPUT_DIR)/$(NAUT_ASM_NAME)

CPPFLAGS += $(NAUT_INCLUDE)

SUPRESSED_WARNINGS += \
		-Wno-frame-address \
	        -Wno-unused-function \
	        -Wno-unused-variable \
		-Wno-unused-but-set-variable \
		-Wno-pointer-sign \

CFLAGS += $(COMMON_FLAGS) \
	  -Wall \
	  $(SUPRESSED_WARNINGS) \
	  -fno-common \
	  -Wstrict-overflow=5 \
	  $(NAUT_INCLUDE)

AFLAGS += $(COMMON_FLAGS) \
	  $(NAUT_INCLUDE)

LDFLAGS += 
MAKEFLAGS += --no-print-directory

#
# Build Rules
#

# Root Directories to generate a builtin.o from
root-builtin-dirs := $(SOURCE_DIR) $(LIBRARY_DIR)

define root-builtin-dir-rule =
$1/builtin.o: $$(AUTOCONF) FORCE
	$$(call quiet-cmd,MAKE,$$(call rel-dir, $$@, $(ROOT_DIR)))
	$$(Q)$$(MAKE) -C $1 -f $$(SCRIPTS_DIR)/build.mk builtin.o
endef
$(foreach root-builtin-dir, $(root-builtin-dirs), $(eval $(call root-builtin-dir-rule,$(root-builtin-dir))))

# The builtin.o files which will be linked together into the kernel
root-builtin := $(addsuffix /builtin.o, $(root-builtin-dirs))

# Kernel Link Rule
$(NAUT_BIN_NAME): $(NAUT_BIN)
$(NAUT_BIN): $(LD_SCRIPT) $(root-builtin) FORCE
	$(call quiet-cmd,LD,$(NAUT_BIN_NAME))
	$(Q)$(LD) $(LDFLAGS) -T$(LD_SCRIPT) \
		$(sort \
		$(wildcard $(SOURCE_DIR)/builtin.o)\
		$(wildcard $(LIBRARY_DIR)/builtin.o)\
		)\
		/dev/null -o $(NAUT_BIN)

# Linker Script Generation
$(LD_SCRIPT): $(LD_SCRIPT_SRC) $(AUTOCONF)
	$(call quiet-cmd,CPP,$@)
	$(Q)$(CPP) -CC -P $(CPPFLAGS) $< -o $@

#
# Utility Rules
#

-include $(ARCH_SCRIPTS_DIR)/rules.mk
-include $(TOOLCHAIN_SCRIPTS_DIR)/rules.mk

ifdef OBJDUMP
asm: $(NAUT_ASM)
$(NAUT_ASM): $(NAUT_BIN)
	$(call quiet-cmd,OBJDUMP,$@)
	$(Q)$(OBJDUMP) -S $(NAUT_BIN) > $@
endif

ifdef QEMU

QEMU_FLAGS += -smp cpus=2
QEMU_FLAGS += -serial stdio
QEMU_FLAGS += -m 2G

qemu: $(QEMU_DEPS)
	$(QEMU) $(QEMU_FLAGS)
qemu-gdb: $(QEMU_DEPS)
	$(QEMU) $(QEMU_FLAGS) -gdb tcp::1234 -S
qemu-gdb-%: $(QEMU_DEPS)
	$(QEMU) $(QEMU_FLAGS) -gdb tcp::$* -S

ifdef NAUT_CONFIG_USE_FDT

QEMU_DTB := $(OUTPUT_DIR)/qemu.dtb
QEMU_DTS := $(OUTPUT_DIR)/qemu.dts

$(QEMU_DTB): $(QEMU_DEPS) FORCE
	$(Q)$(QEMU) $(QEMU_FLAGS) -machine dumpdtb=$(QEMU_DTB)	

DTC := dtc
%.dts: %.dtb
	$(Q)$(DTC) $< -o $@

qemu-dts: $(QEMU_DTS)

device-tree-clean: FORCE
	$(Q)rm -f $(QEMU_DTB)
	$(Q)rm -f $(QEMU_DTS)
CLEAN_RULES += device-tree-clean

endif
endif

DEFAULT_RULES ?= $(NAUT_BIN)
default: $(DEFAULT_RULES)

clean: $(CLEAN_RULES)
	$(Q)find $(SOURCE_DIR) -name "*.o" -delete
	$(Q)find $(SOURCE_DIR) -name "*.cmd" -delete
	$(Q)find $(LIBRARY_DIR) -name "*.o" -delete
	$(Q)find $(LIBRARY_DIR) -name "*.cmd" -delete
	$(Q)find $(OUTPUT_DIR) -name "*.bin" -delete
	$(Q)find $(SCRIPTS_DIR) -name "*.o" -delete
	$(Q)find $(SCRIPTS_DIR) -name "*.cmd" -delete
	$(Q)rm -f $(LD_SCRIPT)
	$(Q)rm -f $(NAUT_BIN)
	$(Q)rm -f $(NAUT_ASM)
	$(Q)rm -f $(AUTOCONF)

FORCE:

.PHONY: default clean $(CLEAN_RULES)

endif

