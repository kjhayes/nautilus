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


#include <dev/pci/of.h>
#include <nautilus/of/dt.h>
#include <dev/pci.h>
#include <nautilus/nautilus.h>
#include <nautilus/iomap.h>

#ifndef NAUT_CONFIG_DEBUG_PCI
#undef DEBUG_PRINT
#define DEBUG_PRINT(fmt, args...) 
#endif

#define PCI_PRINT(fmt, args...) printk("OF-PCI: " fmt, ##args)
#define PCI_DEBUG(fmt, args...) DEBUG_PRINT("OF-PCI: " fmt, ##args)
#define PCI_WARN(fmt, args...)  WARN_PRINT("OF-PCI: " fmt, ##args)
#define PCI_ERROR(fmt, args...) ERROR_PRINT("OF-PCI: " fmt, ##args)

#define PCI_ECAM_SLOT_SHIFT 15
#define PCI_ECAM_BUS_SHIFT 20
#define PCI_ECAM_FUNC_SHIFT 12

#define PCI_CAM_SLOT_SHIFT 11
#define PCI_CAM_BUS_SHIFT 16
#define PCI_CAM_FUNC_SHIFT 8

struct of_pci_state 
{
    void *base_addr;
    size_t size;

    int slot_shift;
    int bus_shift;
    int func_shift;
    uint32_t enable_bit;
};

static size_t 
of_pci_mmio_offset(
        struct of_pci_state *pci_state, 
        uint8_t bus, 
        uint8_t slot, 
        uint8_t fun, 
        uint8_t off) 
{
    size_t offset;
    uint32_t lbus  = (uint32_t)bus;
    uint32_t lslot = (uint32_t)slot;
    uint32_t lfun  = (uint32_t)fun;
    
    offset = (lbus  << pci_state->slot_shift) | 
           (lslot << pci_state->bus_shift) | 
           (lfun  << pci_state->func_shift) |
           PCI_REG_MASK(off) | 
           pci_state->enable_bit;


#ifdef NAUT_CONFIG_ENABLE_ASSERTS
    if((size_t)offset >= pci_state->size) {
        PCI_WARN("Calculated ECAM offset greater than the size of PCI Configuration Space!\n");
    }
#endif

    return offset;
}

static uint16_t 
of_pci_cfg_readw (
               void *state,
               uint8_t bus, 
               uint8_t slot,
               uint8_t fun,
               uint8_t off)
{
    struct of_pci_state *pci_state = (struct of_pci_state*)state;
    uint32_t offset = of_pci_mmio_offset(pci_state, bus, slot, fun, off);
    uint16_t read = *(volatile uint16_t*)(pci_state->base_addr + offset);
    return read;
}


static uint32_t 
of_pci_cfg_readl (
               void *state,
               uint8_t bus, 
               uint8_t slot,
               uint8_t fun,
               uint8_t off)
{
    struct of_pci_state *pci_state = (struct of_pci_state*)state;
    uint32_t offset = of_pci_mmio_offset(pci_state, bus, slot, fun, off);
    uint32_t read = *(volatile uint32_t*)(pci_state->base_addr + offset);
    return read;
}


static void
of_pci_cfg_writew (
        void *state,
        uint8_t bus, 
		uint8_t slot,
		uint8_t fun,
		uint8_t off,
		uint16_t val)
{
    struct of_pci_state *pci_state = (struct of_pci_state*)state;
    uint32_t offset = of_pci_mmio_offset(pci_state, bus, slot, fun, off);
    *(volatile uint16_t*)(pci_state->base_addr + offset) = val;
}


static void
of_pci_cfg_writel (
        void *state,
        uint8_t bus, 
		uint8_t slot,
		uint8_t fun,
		uint8_t off,
		uint32_t val)
{
    struct of_pci_state *pci_state = (struct of_pci_state*)state;
    uint32_t offset = of_pci_mmio_offset(pci_state, bus, slot, fun, off);
    *(volatile uint32_t*)(pci_state->base_addr + offset) = val;
}

struct pci_cfg_int of_pci_cfg_int = {
    .readw = of_pci_cfg_readw,
    .readl = of_pci_cfg_readl,
    .writew = of_pci_cfg_writew,
    .writel = of_pci_cfg_writel,
};

static struct pci_info *of_pci_info = NULL;

static int 
of_pci_init_dev_info(struct nk_dev_info *dev_info) 
{
    struct of_pci_state *state = malloc(sizeof(struct of_pci_state));
    if(state == NULL) {
        PCI_ERROR("Failed to allocate of_pci_state struct!\n");
        return -1;
    }
    memset(state, 0, sizeof(struct of_pci_state));

    const char *name = nk_dev_info_get_name(dev_info);
    PCI_DEBUG("Checking dev_info: \"%s\"...\n", name);

    const char *dev_type = NULL;
    if(nk_dev_info_read_string(dev_info, "device_type", &dev_type)) {
        PCI_ERROR("Failed to get device_type property from PCI node!\n");
        return 1;
    }
    if(strcmp(dev_type, "pci")) {
        PCI_ERROR("device_type = \"%s\", when it should be \"pci\"!\n", dev_type);
        return 1;
    }
            
    // Check if this is a CAM or ECAM pci bus
    int is_ecam = !(strcmp(name, "pci-host-ecam-generic"));
    if(is_ecam) {
        state->bus_shift = PCI_ECAM_BUS_SHIFT;
        state->slot_shift = PCI_ECAM_SLOT_SHIFT;
        state->func_shift = PCI_ECAM_FUNC_SHIFT;
    } else {
        state->bus_shift = PCI_CAM_BUS_SHIFT;
        state->slot_shift = PCI_CAM_SLOT_SHIFT;
        state->func_shift = PCI_CAM_FUNC_SHIFT;
    }

    void *reg_base = NULL;
    size_t reg_size = 0;
    if(nk_dev_info_read_register_block(dev_info, &reg_base, &reg_size)) {
        PCI_ERROR("Failed to read configuration space base address from the device tree!\n");
        return 1;
    }
    state->base_addr = nk_io_map(reg_base, reg_size, 0);
    state->size = reg_size;

    PCI_DEBUG("ECAM MMIO Base Address: %p\n", state->base_addr);
    PCI_DEBUG("ECAM MMIO Size: %p\n", state->size);

    struct pci_info *pci_info = pci_info_create(&of_pci_cfg_int, state);
    if(pci_info && !of_pci_info) {
        of_pci_info = pci_info;
        return 0;
    }

    return 1;
}

static const struct of_dev_id of_pci_dev_ids[] = {
    { .compatible = "pci-host-ecam-generic" },
    { .compatible = "pci-host-cam-generic" },
};

static const struct of_dev_match of_pci_dev_match = {
    .ids = of_pci_dev_ids,
    .num_ids = sizeof(of_pci_dev_ids)/sizeof(struct of_dev_id),
    .max_num_matches = 1,
};

int of_pci_init() {
    if(of_for_each_match(&of_pci_dev_match, of_pci_init_dev_info)) {
        return 1;
    }
    return pci_init(nk_get_nautilus_info(), of_pci_info);
}

