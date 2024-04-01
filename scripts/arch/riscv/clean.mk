
riscv-clean:
	$(Q)rm -f uImage
	$(Q)rm -f Image
	$(Q)rm -f qemu.dtb
	$(Q)rm -f qemu.dts

CLEAN_RULES += riscv-clean

