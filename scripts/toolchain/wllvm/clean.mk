
wllvm-clean:
	$(Q)find $(OUTPUT_DIR) -name "*.bc" -delete
	$(Q)rm -f $(OUTPUT_DIR)nautilus.ll

CLEAN_RULES += wllvm-clean

