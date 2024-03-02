
ifdef NAUT_CONFIG_LLVM_DEBUG_STRIP
build: strip_pass
strip_pass:
	$(call add_llvm_pass --strip-debug)
endif

obj-$(NAUT_CONFIG_CARAT) += carat/
obj-$(NAUT_CONFIG_BEANDIP) += beandip/

