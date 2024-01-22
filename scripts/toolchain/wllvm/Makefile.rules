
bitcode: $(NAUT_BIN)
	# Set up whole kernel bitcode via WLLVM
	$(call quiet-cmd,EXTRACT,$(call rel-file, $(NAUT_BC), $(ROOT_DIR)))
	$(Q)extract-bc $(NAUT_BIN) 
	$(Q)mv $(NAUT_BIN).bc $(NAUT_BC) 
	$(call quiet-cmd,DISAS,$(call rel-file, $(NAUT_LL), $(ROOT_DIR)))
	$(Q)llvm-dis $(NAUT_BC) -o $(NAUT_LL)

wllvm-clean:
	rm -f $(NAUT_BC)
	rm -f $(NAUT_LL)
	$(Q)find $(OUTPUT_DIR) -name "*.bc" -delete

CLEAN_RULES += wllvm-clean

