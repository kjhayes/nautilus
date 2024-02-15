
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

# Compiler-Timing
# Build --- scripts/pass_build.sh compiler-timing CompilerTiming.cpp --- FIX
timing: dep/beandip-linux/local/lib/BEANDIP.so $(BIN_NAME) FORCE
	# Run select loop simplification passes
	opt -basic-aa -mergereturn --break-crit-edges -loop-simplify -mem2reg -simplifycfg-sink-common=false -licm --polly-canonicalize --lowerswitch -lcssa -S $(NAUT_LL) -o $(NAUT_LL).loop.ll
	# Run compiler-timing pass
	clang -Xclang -fpass-plugin=$< -c -emit-llvm $(NAUT_LL).loop.ll -o $(NAUT_LL)
	# opt -load-pass-plugin=$< -passes=Beandip -S $(NAUT_LL).loop.ll -o $(NAUT_LL)

final_no_timing: FORCE
	# Recompile (with full opt levels) new object files, binaries
	clang $(CFLAGS) -c $(NAUT_LL) -o .nautilus.o
	$(LD) $(LDFLAGS) $(LDFLAGS_vmlinux) -o $(BIN_NAME) -T $(LD_SCRIPT) .nautilus.o `scripts/findasm.pl`
	rm .nautilus.o

final: FORCE
	# Recompile (with full opt levels) new object files, binaries
	clang $(CFLAGS) -c $(NAUT_LL) -o .nautilus.o
	$(LD) $(LDFLAGS) $(LDFLAGS_vmlinux) -o $(BIN_NAME) -T $(LD_SCRIPT) .nautilus.o `scripts/findasm.pl`
	rm .nautilus.o

strip: FORCE
	# Strip debug info from instrumented whole kernel bitcode
	opt --strip-debug -S $(NAUT_LL) -o $(NAUT_LL).tmp.ll
	mv $(NAUT_LL).tmp.ll $(NAUT_LL)

ifdef NAUT_CONFIG_USE_WLLVM_WHOLE_OPT
whole_opt: $(BIN_NAME) # FIX --- should be deprecated 
	extract-bc $(BIN_NAME) -o $(NAUT_BC)
	opt -strip-debug $(NAUT_BC)
	clang $(CFLAGS) -c $(NAUT_BC) -o .nautilus.o
	$(LD) $(LDFLAGS) $(LDFLAGS_vmlinux) -o $(BIN_NAME) -T $(LD_SCRIPT) .nautilus.o `scripts/findasm.pl`
	rm .nautilus.o
endif

CLEAN_RULES += wllvm-clean

