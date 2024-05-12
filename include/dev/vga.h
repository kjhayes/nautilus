#ifndef __NAUT_DEV_VGA_H__
#define __NAUT_DEV_VGA_H__

#include <nautilus/naut_types.h>
#include <nautilus/spinlock.h>

// Register Ports and Indices
#define VGA_SEQ_ADDR_REG_PORT 0x03C4
#define VGA_SEQ_DATA_REG_PORT 0x03C5

#define VGA_SEQ_MEM_MODE_EVEN_ODD (1<<0)
#define VGA_SEQ_MEM_MODE_CHAIN_4  (1<<1)

struct vga_ops {
    int(*write_reg)(uint16_t port, uint8_t data);
    int(*read_reg)(uint16_t port, uint8_t *data);
};

struct vga 
{
    struct vga_ops vga_ops;

    spinlock_t lock;
};

static inline int 
vga_write_reg(struct vga *vga, uint16_t port, uint8_t data) {
    return vga->vga_ops.write_reg(port, data);
}
static inline int 
vga_read_reg(struct vga *vga, uint16_t port, uint8_t *data) {
    return vga->vga_ops.read_reg(port, data);
}

int vga_initialize_struct(struct vga *vga, struct vga_ops *ops);

#endif
