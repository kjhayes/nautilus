
bitcode: $(NAUT_BIN)
	# Set up whole kernel bitcode via WLLVM
	$(call quiet-cmd,EXTRACT,$(call rel-file, $(NAUT_BC), $(ROOT_DIR)))
	$(Q)get-bc $(NAUT_BIN) $(QPIPE)
	$(Q)mv $(NAUT_BIN).bc $(NAUT_BC) 
	$(call quiet-cmd,DISAS,$(call rel-file, $(NAUT_LL), $(ROOT_DIR)))
	$(Q)llvm-dis $(NAUT_BC) -o $(NAUT_LL)

passes: bitcode
	$(call quiet-cmd,TRANSFORM,$(call rel-file,$(NAUT_LL),$(ROOT_DIR)))
	bash $(TOOLCHAIN_SCRIPTS_DIR)/apply_passes.sh $(NAUT_LL) $(LLVM_PASS_DIR) $(LLVM_PASS_LIST) $(LLVM_PASS_PLUGIN_LIST)
	$(call quiet-cmd,CLANG,.nautilus.o)
	$(Q)clang $(CFLAGS) -Wno-unused-command-line-argument -c $(NAUT_LL) -o .nautilus.o
	$(call quiet-cmd,LD,$(call rel-file,$(NAUT_BIN),$(ROOT_DIR)))
	$(Q)$(LD) $(LDFLAGS) -o $(NAUT_BIN) -T $(LD_SCRIPT) .nautilus.o `scripts/findasm.pl`
	$(Q)rm .nautilus.o

without-passes: bitcode
	$(Q)clang $(CFLAGS) -c $(NAUT_LL) -o .nautilus.o
	$(Q)$(LD) $(LDFLAGS) -o $(NAUT_BIN) -T $(LD_SCRIPT) .nautilus.o `scripts/findasm.pl`
	$(Q)rm .nautilus.o

