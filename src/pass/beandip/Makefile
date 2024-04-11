
BEANDIP_DIR := $(ROOT_DIR)/dep/beandip-linux

# LLVM Pass
BEANDIP_SO := $(BEANDIP_DIR)/local/lib/BEANDIP.so

beandip: FORCE
	$(Q)$(SCRIPTS_DIR)/build_beandip_linux.sh
	$(call add_llvm_pass,-basic-aa -mergereturn --break-crit-edges -loop-simplify -mem2reg -simplifycfg-sink-common=false -licm --polly-canonicalize --lowerswitch -lcssa)
	$(call add_llvm_pass_plugin,$(BEANDIP_SO))

build: beandip

# Runtime
obj-y += simplified_timehook.o

