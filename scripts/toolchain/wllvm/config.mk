
LLVM_PASS_DIR := $(ROOT_DIR)/llvm
LLVM_PASS_LIST := $(LLVM_PASS_DIR)/passes.txt
LLVM_PASS_PLUGIN_LIST := $(LLVM_PASS_DIR)/passplugins.txt

PASSES_BIN_NAME := nautilus.passes.bin
WITHOUT_PASSES_BIN_NAME := nautilus.without_passes.bin

PASSES_BIN := $(TMPBIN_DIR)/$(PASSES_BIN_NAME)
WITHOUT_PASSES_BIN := $(TMPBIN_DIR)/$(WITHOUT_PASSES_BIN_NAME)

$(shell mkdir -p $(LLVM_PASS_DIR))
$(shell rm -f $(LLVM_PASS_LIST))
$(shell touch $(LLVM_PASS_LIST))
$(shell rm -f $(LLVM_PASS_PLUGIN_LIST))
$(shell touch $(LLVM_PASS_PLUGIN_LIST))

CC := gclang
CXX := gclang++
AS := llvm-as
LD := ld.lld
CPP := cpp

OBJCOPY := llvm-objcopy
OBJDUMP := llvm-objdump

export GLLVM_OBJCOPY:=$(OBJCOPY)

COMMON_FLAGS += -O2  # -fno-delete-null-pointer-checks
# -O3 will also work - PAD

ifdef NAUT_CONFIG_ARCH_X86
COMMON_FLAGS += --target=x86_64
endif

ifdef NAUT_CONFIG_ARCH_RISCV
COMMON_FLAGS += --target=riscv64 \
                -mno-relax
endif

ifdef NAUT_CONFIG_ARCH_ARM64
COMMON_FLAGS += -mcmodel=large \
		--target=aarch64
endif

NAUT_BC := $(OUTPUT_DIR)/nautilus.bc
NAUT_LL := $(OUTPUT_DIR)/nautilus.ll

define add_llvm_lib =
	$(Q)cp $$(1) $(LLVM_PASS_DIR)/$$(2)
endef
define add_llvm_pass =
    $(Q)echo "$$(1)" >> $(LLVM_PASS_LIST)
endef
define add_llvm_pass_plugin =
    $(Q)echo "$$(1)" >> $(LLVM_PASS_PLUGIN_LIST)
endef

DEFAULT_RULES := passes

