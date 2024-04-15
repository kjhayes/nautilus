# ==========================================================================
# Build system
# ==========================================================================

include $(SCRIPTS_DIR)/include.mk

# -mpreferred-stack-boundary=2 is essential in preventing gcc 4.2.x
# from aligning stack to 16 bytes. (Which is gcc's way of supporting SSE).
CFLAGS += $(call cc-option,-m64,) 
AFLAGS += $(call cc-option,-m64,)
ifndef NAUT_CONFIG_XEON_PHI
ifdef NAUT_CONFIG_USE_GCC
LDFLAGS += -melf_x86_64 -dp
endif
endif

COMMON_FLAGS += -mcmodel=large -mno-red-zone

QEMU := qemu-system-x86_64
QEMU_DEPS += $(NAUT_ISO)
QEMU_FLAGS += -cdrom $(NAUT_ISO)
