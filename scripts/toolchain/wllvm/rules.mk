
$(NAUT_LL): $(NAUT_TMP_BIN)
	# Set up whole kernel bitcode via WLLVM
	$(call quiet-cmd,EXTRACT,$(call rel-file, $(NAUT_BC), $(ROOT_DIR)))
	$(Q)get-bc $(NAUT_TMP_BIN) $(QPIPE)
	$(Q)mv $(NAUT_TMP_BIN).bc $(NAUT_BC) 
	$(call quiet-cmd,DISAS,$(call rel-file, $(NAUT_LL), $(ROOT_DIR)))
	$(Q)llvm-dis $(NAUT_BC) -o $(NAUT_LL)

$(PASSES_BIN): $(NAUT_LL)
	$(call quiet-cmd,TRANSFORM,$(call rel-file,$(NAUT_LL),$(ROOT_DIR)))
	bash $(TOOLCHAIN_SCRIPTS_DIR)/apply_passes.sh $(NAUT_LL) $(LLVM_PASS_DIR) $(LLVM_PASS_LIST) $(LLVM_PASS_PLUGIN_LIST)
	$(call quiet-cmd,CLANG,.nautilus.o)
	$(Q)clang $(CFLAGS) -Wno-unused-command-line-argument -c $(NAUT_LL) -o .nautilus.o
	$(call quiet-cmd,LD,$(call rel-file,$(PASSES_BIN),$(ROOT_DIR)))
	$(Q)$(LD) $(LDFLAGS) -o $(PASSES_BIN) -T $(LD_SCRIPT) .nautilus.o `scripts/findasm.pl`
	$(Q)rm .nautilus.o

$(WITHOUT_PASSES_BIN): $(NAUT_LL)
	$(Q)clang $(CFLAGS) -c $(NAUT_LL) -o .nautilus.o
	$(Q)$(LD) $(LDFLAGS) -o $(WITHOUT_PASSES_BIN) -T $(LD_SCRIPT) .nautilus.o `scripts/findasm.pl`
	$(Q)rm .nautilus.o

passes: $(PASSES_BIN)
	@cp $(PASSES_BIN) $(NAUT_BIN)

without-passes: $(WITHOUT_PASSES_BIN)
	@cp $(WITHOUT_PASSES_BIN) $(NAUT_BIN)
