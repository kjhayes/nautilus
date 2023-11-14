# ==========================================================================
# Build system
# ==========================================================================

include $(SCRIPTS_DIR)/Makefile.include

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

GRUBMKRESCUE := grub-mkrescue
GENSYMS := $(ARCH_SCRIPTS_DIR)/gen_sym_file.sh
GENSECS := $(ARCH_SCRIPTS_DIR)/gen_sec_file.sh

NAUT_ISO := $(OUTPUT_DIR)/nautilus.iso
NAUT_SYM := $(OUTPUT_DIR)/nautilus.syms
NAUT_SEC := $(OUTPUT_DIR)/nautilus.secs

isoimage: $(NAUT_ISO) FORCE

$(NAUT_SYM): $(NAUT_BIN)
	$(call quiet_cmd,GENSYMS,$@)
	$(Q)$(GENSYMS) $(NAUT_SYM) tmp.sym

$(NAUT_SEC): $(NAUT_BIN)
	$(call quiet_cmd,GENSECS,$@)
	$(Q)$(GENSECS) $(NAUT_SEC) tmp.sec

$(NAUT_ISO): $(NAUT_BIN) $(NAUT_SYM) $(NAUT_SEC)
	$(call quiet_cmd,MKISO,$@)
	$(Q)mkdir -p .iso/boot/grub && \
	cp configs/grub.cfg .iso/boot/grub && \
	cp $(NAUT_BIN) $(NAUT_SYM) $(NAUT_SEC) .iso/boot && \
	$(GRUBMKRESCUE) -o $(NAUT_ISO) .iso >/dev/null 2>&1 && \
	rm -rf .iso

CLEAN_RULES += x64-clean-isoimage

x64-clean-isoimage:
	$(Q)rm -rf .iso
	$(Q)rm -f tmp.sym
	$(Q)rm -f tmp.sec
	$(Q)rm -f $(NAUT_ISO)
	$(Q)rm -f $(NAUT_SYM)
	$(Q)rm -f $(NAUT_SEC)

FORCE:

