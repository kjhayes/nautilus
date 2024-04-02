
uImage: $(NAUT_BIN)
	$(call quiet-cmd,OBJCOPY,Image)
	$(Q)$(OBJCOPY) -O binary $(NAUT_BIN) Image
	$(call quiet-cmd,MKIMAGE,$@)
	$(Q)mkimage -A riscv -O linux -T kernel -C none \
		-a $(NAUT_CONFIG_KERNEL_LINK_ADDR) -e $(NAUT_CONFIG_KERNEL_LINK_ADDR) -n "Nautilus" \
		-d Image uImage $(QPIPE)
	$(Q)rm Image

