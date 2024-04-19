/* 
 * This file is part of the Nautilus AeroKernel developed
 * by the Hobbes and V3VEE Projects with funding from the 
 * United States National  Science Foundation and the Department of Energy.  
 *
 * The V3VEE Project is a joint project between Northwestern University
 * and the University of New Mexico.  The Hobbes Project is a collaboration
 * led by Sandia National Laboratories that includes several national 
 * laboratories and universities. You can find out more at:
 * http://www.v3vee.org  and
 * http://xstack.sandia.gov/hobbes
 *
 * Copyright (c) 2015, Kyle C. Hale <kh@u.northwestern.edu>
 * Copyright (c) 2017, Peter Dinda <pdinda@northwestern.edu>
 * Copyright (c) 2015, The V3VEE Project  <http://www.v3vee.org> 
 *                     The Hobbes Project <http://xstack.sandia.gov/hobbes>
 * All rights reserved.
 *
 * Authors: Kyle C. Hale <kh@u.northwestern.edu>
 *          Peter Dinda <pdinda@northwestern.edu>
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "LICENSE.txt".
 */

#include <dev/pci.h>
#include <nautilus/naut_types.h>
#include <nautilus/nautilus.h>
#include <nautilus/cpu.h>

#define PCI_CFG_ADDR_PORT 0xcf8
#define PCI_CFG_DATA_PORT 0xcfc

#define PCI_SLOT_SHIFT 11
#define PCI_BUS_SHIFT  16
#define PCI_FUN_SHIFT  8

#define PCI_ENABLE_BIT 0x80000000UL

uint16_t 
pci_cam_cfg_readw (
               void *state,
               uint8_t bus, 
               uint8_t slot,
               uint8_t fun,
               uint8_t off)
{
    uint32_t addr;
    uint32_t lbus  = (uint32_t)bus;
    uint32_t lslot = (uint32_t)slot;
    uint32_t lfun  = (uint32_t)fun;
    uint32_t ret;
    
    addr = (lbus  << PCI_BUS_SHIFT) | 
           (lslot << PCI_SLOT_SHIFT) | 
           (lfun  << PCI_FUN_SHIFT) |
           PCI_REG_MASK(off) | 
           PCI_ENABLE_BIT;

    outl(addr, PCI_CFG_ADDR_PORT);
    ret = inl(PCI_CFG_DATA_PORT);
    return (ret >> ((off & 0x2) * 8)) & 0xffff;
}


uint32_t 
pci_cam_cfg_readl (
               void *state,
               uint8_t bus, 
               uint8_t slot,
               uint8_t fun,
               uint8_t off)
{
    uint32_t addr;
    uint32_t lbus  = (uint32_t)bus;
    uint32_t lslot = (uint32_t)slot;
    uint32_t lfun  = (uint32_t)fun;
    
    addr = (lbus  << PCI_BUS_SHIFT) | 
           (lslot << PCI_SLOT_SHIFT) | 
           (lfun  << PCI_FUN_SHIFT) |
           PCI_REG_MASK(off) | 
           PCI_ENABLE_BIT;

    outl(addr, PCI_CFG_ADDR_PORT);
    return inl(PCI_CFG_DATA_PORT);
}


void
pci_cam_cfg_writew (
        void *state,
        uint8_t bus, 
		uint8_t slot,
		uint8_t fun,
		uint8_t off,
		uint16_t val)
{
    uint32_t addr;
    uint32_t lbus  = (uint32_t)bus;
    uint32_t lslot = (uint32_t)slot;
    uint32_t lfun  = (uint32_t)fun;
    uint32_t ret;
    
    addr = (lbus  << PCI_BUS_SHIFT) | 
           (lslot << PCI_SLOT_SHIFT) | 
           (lfun  << PCI_FUN_SHIFT) |
           PCI_REG_MASK(off) | 
           PCI_ENABLE_BIT;

    outl(addr, PCI_CFG_ADDR_PORT);
    outw(val,PCI_CFG_DATA_PORT);
}


void
pci_cam_cfg_writel (
        void *state,
        uint8_t bus, 
		uint8_t slot,
		uint8_t fun,
		uint8_t off,
		uint32_t val)
{
    uint32_t addr;
    uint32_t lbus  = (uint32_t)bus;
    uint32_t lslot = (uint32_t)slot;
    uint32_t lfun  = (uint32_t)fun;
    
    addr = (lbus  << PCI_BUS_SHIFT) | 
           (lslot << PCI_SLOT_SHIFT) | 
           (lfun  << PCI_FUN_SHIFT) |
           PCI_REG_MASK(off) | 
           PCI_ENABLE_BIT;

    outl(addr, PCI_CFG_ADDR_PORT);
    outl(val, PCI_CFG_DATA_PORT);
}

struct pci_cfg_int pci_cam_cfg_int = {
    .readw = pci_cam_cfg_readw,
    .readl = pci_cam_cfg_readl,
    .writew = pci_cam_cfg_writew,
    .writel = pci_cam_cfg_writel,
};

int port_pci_init(void) {
    struct pci_info *info = pci_info_create(&pci_cam_cfg_int, NULL);
    if(info == NULL) {
        return 1;
    }
    return pci_init(nk_get_nautilus_info(), info);
}

