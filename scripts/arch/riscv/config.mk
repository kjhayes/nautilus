
COMMON_FLAGS += -mcmodel=medany -march=rv64gc -mabi=lp64d

QEMU := qemu-system-riscv64 \
            -bios default \
            -M virt \
            -kernel uImage

QEMU_DEPS += uImage

