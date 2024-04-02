

uImage: $(NAUT_BIN)
	$(call quiet-cmd,OBJCOPY,Image)
	$(Q)$(OBJCOPY) -O binary $< Image
	$(call quiet-cmd,MKIMAGE,$@)
	$(Q)mkimage -A arm64 -O linux -T kernel -C none \
		-a $(NAUT_CONFIG_KERNEL_LINK_ADDR) -e $(NAUT_CONFIG_KERNEL_LINK_ADDR) -n "Nautilus" \
		-d Image uImage $(QPIPE)

qemu-flash: $(QEMU_FLASH)
$(QEMU_FLASH):
	qemu-img create -f raw $(QEMU_FLASH) 64M

#qemu: uImage
#	qemu-system-aarch64 \
#		-smp cpus=4 \
#		--cpu cortex-a72 \
#		-serial stdio \
#		--machine $(QEMU_MACHINE_FLAGS) \
#		-bios $(UBOOT_BIN) \
#		-kernel uImage \
#		$(QEMU_GDB_FLAGS) \
#		$(QEMU_DEVICES) \
#		-numa node,cpus=0-1,memdev=m0 \
#		-numa node,cpus=2-3,memdev=m1 \
#		-object memory-backend-ram,id=m0,size=2G \
#		-object memory-backend-ram,id=m1,size=2G \
#		-m 4G

.PHONY: qemu-flash
