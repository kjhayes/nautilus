
ifdef NAUT_CONFIG_ARCH_X86
GCC_CROSS_COMPILE ?= 
endif

ifdef NAUT_CONFIG_ARCH_ARM64
GCC_CROSS_COMPILE ?= ~/opt/toolchain/aarch64/bin/aarch64-linux-gnu-
LDFLAGS += -m aarch64elf
endif

ifdef NAUT_CONFIG_ARCH_RISCV
GCC_CROSS_COMPILE ?= ~/opt/toolchain/riscv64/bin/riscv64-linux-gnu-
LDFLAGS += -m elf64lriscv_lp64f
endif

CC = $(GCC_CROSS_COMPILE)gcc
LD = $(GCC_CROSS_COMPILE)ld
AS = $(GCC_CROSS_COMPILE)as
CXX = $(GCC_CROSS_COMPILE)g++
CPP = $(GCC_CROSS_COMPILE)cpp
AR = $(GCC_CROSS_COMPILE)ar

OBJCOPY = $(GCC_CROSS_COMPILE)objcopy
OBJDUMP = $(GCC_CROSS_COMPILE)objdump

COMMON_FLAGS += -O2  -fno-delete-null-pointer-checks
GCCVERSIONGTE6 := $(shell expr `$(CC) -dumpversion | cut -f1 -d.` \>= 6)
ifeq "$(GCCVERSIONGTE6)" "1"
  COMMON_FLAGS += -no-pie -fno-pic -fno-PIC -fno-PIE
endif

