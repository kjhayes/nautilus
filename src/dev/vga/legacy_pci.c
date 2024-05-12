
#include <dev/vga.h>
#include <arch/x64/cpu.h>

static int
legacy_pci_vga_read_reg(uint16_t port, uint8_t *data) {
    *data = inb(port);
    return 0;
}
static int
legacy_pci_vga_write_reg(uint16_t port, uint8_t data) {
    outb(data, port);
    return 0;
}

static struct vga_ops
legacy_pci_vga_ops = {
    .read_reg = legacy_pci_vga_read_reg,
    .write_reg = legacy_pci_vga_write_reg,
};

