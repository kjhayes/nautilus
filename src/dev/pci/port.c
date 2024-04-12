
#include <dev/

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

