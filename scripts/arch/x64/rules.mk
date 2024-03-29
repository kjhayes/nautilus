# ==========================================================================
# Build system
# ==========================================================================

include $(SCRIPTS_DIR)/include.mk

GRUBMKRESCUE := grub-mkrescue
GENSYMS := $(ARCH_SCRIPTS_DIR)/gen_sym_file.sh
GENSECS := $(ARCH_SCRIPTS_DIR)/gen_sec_file.sh

NAUT_ISO := $(OUTPUT_DIR)/nautilus.iso

# KJH - I really want to avoid needing to worry about
#       HOSTCC's at all because it makes cross compilation
#       more easy to mess up, not a big deal though
#       (If someone wants to rewrite format_syms/format_secs 
#        in python though be my guest)
HOSTCC := gcc

ifdef NAUT_CONFIG_PROVENANCE
FORMAT_SYMS := $(ARCH_SCRIPTS_DIR)/format_syms
FORMAT_SECS := $(ARCH_SCRIPTS_DIR)/format_secs
NAUT_SYM := $(OUTPUT_DIR)/nautilus.syms
NAUT_SEC := $(OUTPUT_DIR)/nautilus.secs
endif

isoimage: $(NAUT_ISO) FORCE

ifdef NAUT_CONFIG_PROVENANCE

$(FORMAT_SYMS): $(ARCH_SCRIPTS_DIR)/format_syms.c
	$(call quiet-cmd,HOSTCC,$@)
	$(Q)$(HOSTCC) $^ -o $@

$(FORMAT_SECS): $(ARCH_SCRIPTS_DIR)/format_secs.c
	$(call quiet-cmd,HOSTCC,$@)
	$(Q)$(HOSTCC) $^ -o $@

$(NAUT_SYM): $(FORMAT_SYMS) $(NAUT_BIN)
	$(call quiet-cmd,GENSYMS,$@)
	$(Q)$(GENSYMS) $(NAUT_BIN) tmp.sym

$(NAUT_SEC): $(FORMAT_SECS) $(NAUT_BIN)
	$(call quiet-cmd,GENSECS,$@)
	$(Q)$(GENSECS) $(NAUT_BIN) tmp.sec
endif

$(NAUT_ISO): $(NAUT_BIN) $(NAUT_SYM) $(NAUT_SEC)
	$(call quiet-cmd,MKISO,$@)
	$(Q)mkdir -p .iso/boot/grub
	$(Q)cp configs/grub.cfg .iso/boot/grub
	$(Q)cp $(NAUT_BIN) $(NAUT_SYM) $(NAUT_SEC) .iso/boot
	$(Q)$(GRUBMKRESCUE) -o $(NAUT_ISO) .iso >/dev/null 2>&1
	$(Q)rm -rf .iso

DEFAULT_RULES += $(NAUT_ISO)

qemu: $(NAUT_ISO)
	qemu-system-x86_64 -cdrom $(NAUT_ISO) -serial stdio

CLEAN_RULES += x64-clean-isoimage

x64-clean-isoimage:
	$(Q)rm -rf .iso
	$(Q)rm -f tmp.sym
	$(Q)rm -f tmp.sec
	$(Q)rm -f $(NAUT_ISO)
	$(Q)rm -f $(NAUT_SYM)
	$(Q)rm -f $(NAUT_SEC)

FORCE:

