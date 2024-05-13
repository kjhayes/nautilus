/*
 * This file is part of the Nautilus AeroKernel developed
 * by the Constellation, Interweaving, Hobbes, and V3VEE
 * Projects with funding from the United States National
 * Science Foundation and the Department of Energy.
 *
 * The V3VEE Project is a joint project between Northwestern University
 * and the University of New Mexico.  The Hobbes Project is a collaboration
 * led by Sandia National Laboratories that includes several national
 * laboratories and universities. The Interweaving Project is a
 * joint project between Northwestern University and Illinois Institute
 * of Technology.   The Constellation Project is a joint project
 * between Northwestern University, Carnegie Mellon University,
 * and Illinois Institute of Technology.
 * You can find out more at:
 * http://www.v3vee.org
 * http://xstack.sandia.gov/hobbes
 * http://interweaving.org
 * http://constellation-project.net
 *
 * Copyright (c) 2023, Kevin Hayes <kjhayes@u.northwestern.edu>
 * Copyright (c) 2023, The V3VEE Project  <http://www.v3vee.org>
 *                     The Hobbes Project <http://xstack.sandia.gov/hobbes>
 * All rights reserved.
 *
 * Authors: Kevin Hayes <kjhayes@u.northwestern.edu>
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "LICENSE.txt".
 */

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
