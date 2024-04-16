
#
# Include .config early so that we have access to the toolchain-related options

export
DOT_CONFIG := $(ROOT_DIR)/.config
AUTOCONF := $(INCLUDE_DIR)/autoconf.h
KCONFIG_DIR := $(SCRIPTS_DIR)/kconfig

NAUT_RED := \#B13B3C
NAUT_WHITE := \#FFFFFF
NAUT_LIGHT_GREY := \#626262
NAUT_DARK_GREY := \#222122
NAUT_BLACK := \#000000

NAUTILUS_MENUCONFIG_STYLE := "\
	default \
    path=fg:$(NAUT_WHITE),bg:$(NAUT_RED),bold\
    separator=fg:$(NAUT_RED),bg:$(NAUT_WHITE),bold\
    list=fg:$(NAUT_RED),bg:$(NAUT_WHITE)\
    selection=fg:$(NAUT_WHITE),bg:$(NAUT_RED),bold\
    inv-list=fg:$(NAUT_RED),bg:$(NAUT_WHITE)\
    inv-selection=fg:$(NAUT_RED),bg:$(NAUT_WHITE)\
	help=fg:$(NAUT_BLACK),bg:$(NAUT_LIGHT_GREY)\
	show-help=fg:$(NAUT_BLACK),bg:$(NAUT_LIGHT_GREY)\
    frame=fg:$(NAUT_BLACK),bg:$(NAUT_RED),bold\
    body=fg:$(NAUT_WHITE),bg:$(NAUT_BLACK)\
    edit=fg:$(NAUT_WHITE),bg:$(NAUT_RED)\
    jump-edit=edit\
    text=list\
	"

$(AUTOCONF): $(DOT_CONFIG)
	$(call quiet-cmd,GENCONFIG,$(call rel-file, $@, $(ROOT_DIR)))
	$(Q)$(PYTHON) $(KCONFIG_DIR)/genconfig.py --header-path $@ --sync-dep $(INCLUDE_DIR)/config/

menuconfig:
	$(Q)MENUCONFIG_STYLE=$(NAUTILUS_MENUCONFIG_STYLE) $(PYTHON) $(KCONFIG_DIR)/menuconfig.py

defconfig:
	$(Q)$(PYTHON) $(KCONFIG_DIR)/defconfig.py $(SETUPS_DIR)/$(ARCH)/$@

%defconfig:
	$(Q)$(PYTHON) $(KCONFIG_DIR)/defconfig.py $(SETUPS_DIR)/$@

-include $(DOT_CONFIG)

.PHONY: menuconfig defconfig
