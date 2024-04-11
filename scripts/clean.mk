
default-clean:
	$(Q)find $(SOURCE_DIR) -name "*.o" -delete
	$(Q)find $(SOURCE_DIR) -name "*.cmd" -delete
	$(Q)find $(LIBRARY_DIR) -name "*.o" -delete
	$(Q)find $(LIBRARY_DIR) -name "*.cmd" -delete
	$(Q)find $(OUTPUT_DIR) -name "*.bin" -delete
	$(Q)find $(SCRIPTS_DIR) -name "*.o" -delete
	$(Q)find $(SCRIPTS_DIR) -name "*.cmd" -delete
	$(Q)rm -f $(LD_SCRIPT)
	$(Q)rm -f $(NAUT_BIN)
	$(Q)rm -f $(NAUT_ASM)
	$(Q)rm -f $(AUTOCONF)
	$(Q)rm -f $(ROOT_DIR)/builtin.o
	$(Q)rm -rf $(TMPBIN_DIR)

CLEAN_RULES += default-clean

