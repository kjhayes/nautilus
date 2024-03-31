
uImage: $(NAUT_BIN)
	$(call quiet-cmd,OBJCOPY,Image)
	$(Q)$(OBJCOPY) -O binary $(NAUT_BIN) Image
	$(call quiet-cmd,MKIMAGE,$@)
	$(Q)mkimage -A riscv -O linux -T kernel -C none \
		-a $(NAUT_CONFIG_KERNEL_LINK_ADDR) -e $(NAUT_CONFIG_KERNEL_LINK_ADDR) -n "Nautilus" \
		-d Image uImage
	$(Q)rm Image

riscv-clean:
	$(Q)rm -f uImage
	$(Q)rm -f Image
	$(Q)rm -f qemu.dtb
	$(Q)rm -f qemu.dts

DEFAULT_RULES += uImage
CLEAN_RULES += riscv-clean

