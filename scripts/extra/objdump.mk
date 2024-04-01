
ifdef OBJDUMP
asm: $(NAUT_ASM)
$(NAUT_ASM): $(NAUT_BIN)
	$(call quiet-cmd,OBJDUMP,$@)
	$(Q)$(OBJDUMP) -S $(NAUT_BIN) > $@
endif

