
#
# Include .config early so that we have access to the toolchain-related options

export
DOT_CONFIG := $(ROOT_DIR)/.config
AUTOCONF := $(INCLUDE_DIR)/autoconf.h
KCONFIG_DIR := $(SCRIPTS_DIR)/kconfig

$(AUTOCONF): $(DOT_CONFIG)
	$(call quiet-cmd,GENCONFIG,$(call rel-file, $@, $(ROOT_DIR)))
	$(Q)$(PYTHON) $(KCONFIG_DIR)/genconfig.py --header-path $@ --sync-dep $(INCLUDE_DIR)/config/

menuconfig:
	$(Q)$(PYTHON) $(KCONFIG_DIR)/menuconfig.py

defconfig:
	$(Q)$(PYTHON) $(KCONFIG_DIR)/defconfig.py $(SETUPS_DIR)/$(ARCH)/$@

%defconfig:
	$(Q)$(PYTHON) $(KCONFIG_DIR)/defconfig.py $(SETUPS_DIR)/$@

-include $(DOT_CONFIG)

.PHONY: menuconfig defconfig
