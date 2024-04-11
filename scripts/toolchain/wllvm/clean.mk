
wllvm-clean:
	$(Q)find $(OUTPUT_DIR) -name "*.bc" -delete
	$(Q)rm -f $(OUTPUT_DIR)/nautilus.ll
	$(Q)rm -rf $(LLVM_PASS_DIR)

CLEAN_RULES += wllvm-clean

