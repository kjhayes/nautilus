
-include $(SCRIPTS_DIR)/toolchain/wllvm/include.mk

$(shell mkdir -p $(LLVM_PASS_DIR))
$(shell rm -f $(LLVM_PASS_LIST))
$(shell touch $(LLVM_PASS_LIST))
$(shell rm -f $(LLVM_PASS_PLUGIN_LIST))
$(shell touch $(LLVM_PASS_PLUGIN_LIST))
$(shell rm -f $(CLANG_PASS_PLUGIN_LIST))
$(shell touch $(CLANG_PASS_PLUGIN_LIST))

CC := gclang
CXX := gclang++
AS := llvm-as
LD := ld.lld
CPP := cpp

OBJCOPY := llvm-objcopy
OBJDUMP := llvm-objdump

export GLLVM_OBJCOPY:=$(OBJCOPY)

COMMON_FLAGS += -O$(NAUT_CONFIG_COMPILER_OPT_LEVEL)  # -fno-delete-null-pointer-checks
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
define add_clang_pass_plugin =
    $(Q)echo "$$(1)" >> $(CLANG_PASS_PLUGIN_LIST)
endef

OVERRIDE_LINK_RULE := y

