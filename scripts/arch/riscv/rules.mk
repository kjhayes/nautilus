
uImage: $(NAUT_BIN)
	$(call quiet-cmd,OBJCOPY,Image)
	$(Q)$(OBJCOPY) -O binary $(NAUT_BIN) Image
	$(call quiet-cmd,MKIMAGE,$@)
	$(Q)mkimage -A riscv -O linux -T kernel -C none \
		-a $(NAUT_CONFIG_KERNEL_LINK_ADDR) -e $(NAUT_CONFIG_KERNEL_LINK_ADDR) -n "Nautilus" \
		-d Image uImage
	$(Q)rm Image

qemu: uImage
	qemu-system-riscv64 \
            -bios default \
		    -smp cpus=1 \
            -m 2G \
            -M virt \
            -kernel uImage \
            -serial stdio \
            -gdb tcp\:\:1234

riscv-clean:
	$(Q)rm -f uImage
	$(Q)rm -f Image

DEFAULT_RULES += uImage
CLEAN_RULES += riscv-clean

