
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
$(NAUT_BIN): $(LD_SCRIPT) $(root-builtin)
	$(call quiet-cmd,LD,$(NAUT_BIN_NAME))
	$(Q)$(LD) $(LDFLAGS) -T$(LD_SCRIPT) \
		$(sort \
		$(wildcard $(SOURCE_DIR)/builtin.o)\
		$(wildcard $(LIBRARY_DIR)/builtin.o)\
		)\
	    -o $(NAUT_BIN)

# Linker Script Generation
$(LD_SCRIPT): $(LD_SCRIPT_SRC) $(AUTOCONF)
	$(call quiet-cmd,CPP,$@)
	$(Q)$(CPP) -CC -P $(CPPFLAGS) $< -o $@

# Architecture specific rules
-include $(ARCH_SCRIPTS_DIR)/rules.mk
# Toolchain specific rules
-include $(TOOLCHAIN_SCRIPTS_DIR)/rules.mk

#
# Utility Rules
#

# Extra rules for generating disassembly
-include $(SCRIPTS_DIR)/extra/objdump.mk
# Extra rules for running QEMU
-include $(SCRIPTS_DIR)/extra/qemu.mk

# Include clean.mk files found recursively in the scripts dir
# (Regardless of the Kconfig, we want to clean everything we can on make clean,
#  e.g. clean isoimage even without ARCH_X86)
CLEAN_FILES := $(shell find $(SCRIPTS_DIR) -name "clean.mk")
$(foreach CLEAN_FILE,$(CLEAN_FILES), $(eval -include $(CLEAN_FILE)))

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

