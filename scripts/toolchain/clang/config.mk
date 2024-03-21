
CC = clang
LD = ld.lld
AS = clang-as
CXX = clang++
CPP = cpp

OBJCOPY = llvm-objcopy
OBJDUMP = llvm-objdump

COMMON_FLAGS += -O2  # -fno-delete-null-pointer-checks
# -O3 will also work - PAD

ifdef NAUT_CONFIG_ARCH_X86
COMMON_FLAGS += -mcmodel=large \
		--target=x86_64
LDFLAGS += -m elf_x86_64
endif

ifdef NAUT_CONFIG_ARCH_RISCV
COMMON_FLAGS += -mcmodel=large \
		--target=riscv64 \
                -mno-relax
endif

ifdef NAUT_CONFIG_ARCH_ARM64
COMMON_FLAGS += -mcmodel=large \
		--target=aarch64
endif
