
x64-clean-isoimage:
	$(Q)rm -rf .iso
	$(Q)rm -f tmp.sym
	$(Q)rm -f tmp.sec
	$(Q)rm -f nautilus.iso
	$(Q)rm -f nautilus.sym
	$(Q)rm -f nautilus.secs

CLEAN_RULES += x64-clean-isoimage

