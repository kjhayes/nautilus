
CC := wllvm
CXX := wllvm++
AS := llvm-as
LD := ld.lld
CPP := cpp

OBJCOPY := llvm-objcopy
OBJDUMP := llvm-objdump

COMMON_FLAGS += -O2  # -fno-delete-null-pointer-checks
# -O3 will also work - PAD

ifdef NAUT_CONFIG_ARCH_X86
COMMON_FLAGS += --target=x64
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

