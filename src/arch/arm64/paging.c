
#define PAGE_SIZE_4KB 0x1000
#define PAGE_SIZE_16KB 0x4000
#define PAGE_SIZE_64KB 0x10000

#include<nautilus/naut_types.h>
#include<nautilus/nautilus.h>
#include<nautilus/fdt/fdt.h>
#include<nautilus/macros.h>

#include<arch/arm64/sys_reg.h>

#ifndef NAUT_CONFIG_DEBUG_PAGING
#undef DEBUG_PRINT
#define DEBUG_PRINT(fmt, args...)
#endif

#define PAGING_PRINT(fmt, args...) INFO_PRINT("paging: " fmt, ##args)
#define PAGING_DEBUG(fmt, args...) DEBUG_PRINT("paging: " fmt, ##args)
#define PAGING_ERROR(fmt, args...) ERROR_PRINT("paging: " fmt, ##args)
#define PAGING_WARN(fmt, args...) WARN_PRINT("paging: " fmt, ##args)

#define TCR_TTBR0_GRANULE_SIZE_4KB 0b00
#define TCR_TTBR0_GRANULE_SIZE_16KB 0b10
#define TCR_TTBR0_GRANULE_SIZE_64KB 0b01

#define TCR_TTBR1_GRANULE_SIZE_4KB 0b10
#define TCR_TTBR1_GRANULE_SIZE_16KB 0b01
#define TCR_TTBR1_GRANULE_SIZE_64KB 0b11

#define NON_SHAREABLE    0b00
#define INNER_SHAREABLE  0b11
#define OUTER_SHAREABLE  0b10

#define TCR_DEFAULT_CACHEABILITY 0b11
#define TCR_DEFAULT_SHAREABILITY 0b11


typedef union tc_reg {
  uint64_t raw;
  struct {
    uint_t low_mem_msb_mask : 6;
    uint_t __res0_0 : 1;
    uint_t ttbr0_do_not_walk : 1;
    uint_t ttbr0_table_inner_cacheability : 2;
    uint_t ttbr0_table_outer_cacheability : 2;
    uint_t ttbr0_table_shareability : 2;
    uint_t ttbr0_granule_size : 2;
    uint_t high_mem_msb_mask : 6;
    uint_t high_mem_defines_asid : 1;
    uint_t ttbr1_do_not_walk : 1;
    uint_t ttbr1_table_inner_cacheability : 2;
    uint_t ttbr1_table_outer_cacheability : 2;
    uint_t ttbr1_table_shareability : 2;
    uint_t ttbr1_granule_size : 2;
    uint_t ips_size : 3;
    uint_t __res0_1 : 1;
    uint_t asid_16bit : 1;
    uint_t ttbr0_top_bit_ignored : 1;
    uint_t ttbr1_top_bit_ignored : 1;
    uint_t hw_update_acc_flag : 1;
    uint_t hw_update_dirty_bit : 1;
    uint_t ttbr0_disable_hierarchy : 1;
    uint_t ttbr1_disable_hierarchy : 1;
    // There are more fields but these are more than enough
  };
} tc_reg_t;

_Static_assert(sizeof(tc_reg_t) == 8, "TCR structure is not exactly 8 bytes wide!\n");

#define PAGE_TABLE_DESC_TYPE_INVALID 0b00
#define PAGE_TABLE_DESC_TYPE_BLOCK 0b01
#define PAGE_TABLE_DESC_TYPE_TABLE 0b11

#define PAGE_TABLE_DESC_TYPE_L3_VALID 0b11

typedef union page_table_descriptor {
  uint64_t raw;
  struct {
    uint_t desc_type : 2;
    uint_t mair_index : 3;
    uint_t non_secure : 1;
    uint_t access_perm : 2;
    uint_t shareable_attr : 2;
    uint_t access_flag : 1;
    uint_t not_global : 1;

    uint64_t address_data : 36;

    uint_t __res0_1 : 4;

    uint_t priv_exec_never : 1;
    uint_t unpriv_exec_never : 1;
    uint_t extra_data : 4;
    uint_t __res0_2 : 6;
  };
} page_table_descriptor_t;

_Static_assert(sizeof(page_table_descriptor_t) == 8, "Page table descriptor structure is not 8 bytes wide!\n");

#define TABLE_DESC_PTR(table) (page_table_descriptor_t*)((table).raw & 0x0000FFFFFFFFF000)

#define L0_INDEX_4KB(addr) ((((uint64_t)addr) >> 39) & 0x1FF)
#define L1_INDEX_4KB(addr) ((((uint64_t)addr) >> 30) & 0x1FF)
#define L2_INDEX_4KB(addr) ((((uint64_t)addr) >> 21) & 0x1FF)
#define L3_INDEX_4KB(addr) ((((uint64_t)addr) >> 12) & 0x1FF)
#define PAGE_OFFSET_4KB(addr) ((uint64_t)addr) & ((1<<12) - 1)
#define PAGE_OFFSET_2MB(addr) ((uint64_t)addr) & ((1<<21) - 1)
#define PAGE_OFFSET_1GB(addr) ((uint64_t)addr) & ((1<<30) - 1)

#define ALIGNED_4KB(addr) ((((uint64_t)addr) & ((1<<12) - 1)) == 0)
#define ALIGNED_2MB(addr) ((((uint64_t)addr) & ((1<<21) - 1)) == 0)
#define ALIGNED_1GB(addr) ((((uint64_t)addr) & ((1<<30) - 1)) == 0)

#define L1_BLOCK_SIZE_4KB (1<<30) //1 GB
#define L2_BLOCK_SIZE_4KB (1<<21) //2 MB
#define L3_BLOCK_SIZE_4KB (1<<12) //4 KB

// These are some common ones
#define MAIR_ATTR_DEVICE_nGnRnE 0b00000000
#define MAIR_ATTR_NORMAL_NON_CACHEABLE 0b01000100
#define MAIR_ATTR_NORMAL_INNER_OUTER_WB_CACHEABLE 0b11111111

#define MAIR_NORMAL_INDEX 0
#define MAIR_DEVICE_INDEX 1

typedef union mair_reg {
  uint64_t raw;
  uint8_t attrs[8];
} mair_reg_t;

_Static_assert(sizeof(mair_reg_t) == 8, "MAIR Register Structure is not 8 bytes wide!");

static inline int block_to_table(page_table_descriptor_t *block_desc, uint_t table_level) {

  PAGING_DEBUG("block_to_table\n");
  if(table_level == 3) {
    PAGING_WARN("Trying to turn L3 block into a table!\n");
    return -1;
  }

  uint64_t new_block_size;
  switch(table_level) {
    case 0:
      new_block_size = L1_BLOCK_SIZE_4KB;
      break;
    case 1:
      new_block_size = L2_BLOCK_SIZE_4KB;
      break;
    case 2:
      new_block_size = L3_BLOCK_SIZE_4KB;
      break;
    default:
      new_block_size = 0;
      break;
  }

  PAGING_DEBUG("\tNew Block Size = 0x%x\n", new_block_size);

  if(block_desc->desc_type == PAGE_TABLE_DESC_TYPE_BLOCK ||
     block_desc->desc_type == PAGE_TABLE_DESC_TYPE_INVALID) {
   
    uint64_t paddr = (uint64_t)TABLE_DESC_PTR(*block_desc);
    PAGING_DEBUG("\tCurrent PADDR = %p\n", paddr);
 
    page_table_descriptor_t *new_table = malloc(sizeof(page_table_descriptor_t) * 512);
   
    if(new_table == NULL) {
      return -1;
    }

    if((uint64_t)new_table & ((sizeof(page_table_descriptor_t) * 512)-1)) {
      PAGING_ERROR("Block to Table Allocation Produced Unaligned Table!\n");
      free(new_table);
      return -1;
    }

    for(uint32_t i = 0; i < 512; i++) {

      new_table[i].raw = block_desc->raw;

      if(table_level == 2) {
        switch(block_desc->desc_type) {
          case PAGE_TABLE_DESC_TYPE_INVALID:
            new_table[i].desc_type = PAGE_TABLE_DESC_TYPE_INVALID;
            break;
          case PAGE_TABLE_DESC_TYPE_BLOCK:
          default:
            new_table[i].desc_type = PAGE_TABLE_DESC_TYPE_L3_VALID;
            break;
        }
      }

      new_table[i].address_data = 0;
      new_table[i].raw |= ((uint64_t)TABLE_DESC_PTR(*block_desc) + (new_block_size * i));

      PAGING_DEBUG("\tDrilled new entry: PADDR = %p, RAW = 0x%016x\n", TABLE_DESC_PTR(new_table[i]), new_table[i].raw);
    }

    PAGING_DEBUG("Original Block: PADDR = %p, RAW = 0x%016x\n", TABLE_DESC_PTR(*block_desc), block_desc->raw);

    block_desc->desc_type = PAGE_TABLE_DESC_TYPE_TABLE;
    block_desc->address_data = 0;
    block_desc->raw |= (uint64_t)new_table;

    PAGING_DEBUG("TABLE PADDR = %p, DESC_PTR = %p, RAW = 0x%016x\n", new_table, TABLE_DESC_PTR(*block_desc), block_desc->raw);
  }

  return 0;
}

static inline page_table_descriptor_t *drill_page_4kb(page_table_descriptor_t *table, void *vaddr)
{
  PAGING_DEBUG("drill_page_4kb\n");
  table += L0_INDEX_4KB(vaddr);

  if(table->desc_type != PAGE_TABLE_DESC_TYPE_TABLE) {
    PAGING_DEBUG("\tneed to drill 1GB pages first\n");
    block_to_table(table, 0);
  }

  table = TABLE_DESC_PTR(*table);
  table += L1_INDEX_4KB(vaddr);

  if(table->desc_type != PAGE_TABLE_DESC_TYPE_TABLE) {
    PAGING_DEBUG("\tneed to drill 2MB pages first\n");
    block_to_table(table, 1);
  }

  table = TABLE_DESC_PTR(*table);
  table += L2_INDEX_4KB(vaddr);

  if(table->desc_type != PAGE_TABLE_DESC_TYPE_TABLE) {
    PAGING_DEBUG("\tneed to drill 4KB pages\n");
    block_to_table(table, 2);
  } else {
    PAGING_DEBUG("\tno drilling needed\n");
  }

  table = TABLE_DESC_PTR(*table);
  table += L3_INDEX_4KB(vaddr);

  return table;
}

static inline page_table_descriptor_t *drill_page_2mb(page_table_descriptor_t *table, void *vaddr)
{
  PAGING_DEBUG("drill_page_2mb\n");
  table += L0_INDEX_4KB(vaddr);

  if(table->desc_type != PAGE_TABLE_DESC_TYPE_TABLE) {
    PAGING_DEBUG("\tneed to drill 1GB pages first\n");
    block_to_table(table, 0);
  }

  table = TABLE_DESC_PTR(*table);
  table += L1_INDEX_4KB(vaddr);

  if(table->desc_type != PAGE_TABLE_DESC_TYPE_TABLE) {
    PAGING_DEBUG("\tneed to drill 2MB pages\n");
    block_to_table(table, 1);
  } else {
    PAGING_DEBUG("\tno drilling needed\n");
  }

  table = TABLE_DESC_PTR(*table);
  table += L2_INDEX_4KB(vaddr);

  return table;
}

static inline page_table_descriptor_t *drill_page_1gb(page_table_descriptor_t *table, void *vaddr)
{
  PAGING_DEBUG("drill_page_1gb\n");
  table += L0_INDEX_4KB(vaddr);

  if(table->desc_type != PAGE_TABLE_DESC_TYPE_TABLE) {
    PAGING_DEBUG("\tneed to drill 1GB pages\n");
    block_to_table(table, 0);
  } else {
    PAGING_DEBUG("\tno drilling needed\n");
  }

  table = TABLE_DESC_PTR(*table);
  table += L1_INDEX_4KB(vaddr);

 return table;
}

#define VADDR_L0_4KB(i0) (i0 << 39)
#define VADDR_L1_4KB(i0, i1) (i0 << 39 | i1 << 30)
#define VADDR_L2_4KB(i0, i1, i2) (i0 << 39 | i1 << 30 | i2 << 21)
#define VADDR_L3_4KB(i0, i1, i2, i3) (i0 << 39 | i1 << 30 | i2 << 21 | i3 << 12)

static void dump_tc_reg(tc_reg_t tc) {

#define PF(field) PAGING_PRINT("TCR_EL1." #field " = 0x%x\n", tc.field) 

    PF(low_mem_msb_mask);
    PF(ttbr0_do_not_walk);
    PF(ttbr0_table_inner_cacheability);
    PF(ttbr0_table_outer_cacheability);
    PF(ttbr0_table_shareability);
    PF(ttbr0_granule_size);
    PF(high_mem_msb_mask);
    PF(high_mem_defines_asid);
    PF(ttbr1_do_not_walk);
    PF(ttbr1_table_inner_cacheability);
    PF(ttbr1_table_outer_cacheability);
    PF(ttbr1_table_shareability);
    PF(ttbr1_granule_size);
    PF(ips_size);
    PF(asid_16bit);
    PF(ttbr0_top_bit_ignored);
    PF(ttbr1_top_bit_ignored);
    PF(hw_update_acc_flag);
    PF(hw_update_dirty_bit);
    PF(ttbr0_disable_hierarchy);
    PF(ttbr1_disable_hierarchy);

#undef PF

}

static void dump_page_table(page_table_descriptor_t *l0_table) {
  for(uint64_t i0 = 0; i0 < 512; i0++) {
    if(l0_table[i0].desc_type == PAGE_TABLE_DESC_TYPE_TABLE) {
      page_table_descriptor_t *l1_table = TABLE_DESC_PTR(l0_table[i0]);
      for(uint64_t i1 = 0; i1 < 512; i1++) {
        if(l1_table[i1].desc_type == PAGE_TABLE_DESC_TYPE_TABLE) {
          page_table_descriptor_t *l2_table = TABLE_DESC_PTR(l1_table[i1]);
          for(uint64_t i2 = 0; i2 < 512; i2++) {
            if(l2_table[i2].desc_type == PAGE_TABLE_DESC_TYPE_TABLE) {
              page_table_descriptor_t *l3_table = TABLE_DESC_PTR(l2_table[i2]);
              for(uint64_t i3 = 0; i3 < 512; i3++) {
                PAGING_PRINT("\t\t\tVA[0x%016x - 0x%016x] -> PA[0x%016x - 0x%016x] 4KB%s%s%s%s%s\n",
                    VADDR_L3_4KB(i0, i1, i2, i3),
                    VADDR_L3_4KB(i0, i1, i2, i3)+PAGE_SIZE_4KB,
                    (uint64_t)TABLE_DESC_PTR(l3_table[i3]),
                    (uint64_t)TABLE_DESC_PTR(l3_table[i3])+PAGE_SIZE_4KB,
                    l3_table[i3].desc_type == PAGE_TABLE_DESC_TYPE_L3_VALID ? 
                    (l3_table[i3].mair_index == MAIR_DEVICE_INDEX ? " Device" : " Normal") : " Invalid",
                    l3_table[i3].priv_exec_never ? " PXN" : "",
                    l3_table[i3].unpriv_exec_never ? " UXN" : "",
                    l3_table[i3].access_perm & 0b10 ? " RD_ONLY" : "",
                    l3_table[i3].access_flag ? " ACCESS" : ""
                    );
              }
            } else {
              PAGING_PRINT("\t\tVA[0x%016x - 0x%016x] -> PA[0x%016x - 0x%016x] 2MB%s%s%s%s%s\n",
                  VADDR_L2_4KB(i0, i1, i2),
                  VADDR_L2_4KB(i0, i1, i2)+PAGE_SIZE_2MB,
                  (uint64_t)TABLE_DESC_PTR(l2_table[i2]),
                  (uint64_t)TABLE_DESC_PTR(l2_table[i2])+PAGE_SIZE_2MB,
                  l2_table[i2].desc_type == PAGE_TABLE_DESC_TYPE_BLOCK ? 
                  (l2_table[i2].mair_index == MAIR_DEVICE_INDEX ? " Device" : " Normal") : " Invalid",
                  l2_table[i2].priv_exec_never ? " PXN " : "",
                  l2_table[i2].unpriv_exec_never ? " UXN " : "",
                  l2_table[i2].access_perm & 0b10 ? " RD_ONLY" : "",
                  l2_table[i2].access_flag ? " ACCESS" : ""
                  );
            }
          }
        } else {
          PAGING_PRINT("\t[0x%016x - 0x%016x] -> PA[0x%016x - 0x%016x] 1GB%s%s%s%s%s\n", 
              VADDR_L1_4KB(i0, i1), 
              VADDR_L1_4KB(i0, i1)+PAGE_SIZE_1GB,
              (uint64_t)TABLE_DESC_PTR(l1_table[i1]),
              (uint64_t)TABLE_DESC_PTR(l1_table[i1])+PAGE_SIZE_1GB,
              l1_table[i1].desc_type == PAGE_TABLE_DESC_TYPE_BLOCK ?
              (l1_table[i1].mair_index == MAIR_DEVICE_INDEX ? " Device" : " Normal") : " Invalid",
              l1_table[i1].priv_exec_never ? " PXN" : "",
              l1_table[i1].unpriv_exec_never ? " UXN" : "",
              l1_table[i1].access_perm & 0b10 ? " RD_ONLY" : "",
              l1_table[i1].access_flag ? " ACCESS" : ""
              );
        }
      }
    }
    else {
      PAGING_PRINT("[0x%016x - 0x%016x] 512GB Invalid\n", VADDR_L0_4KB(i0), VADDR_L0_4KB(i0)+(((uint64_t)1<<39) - 1));
    }
  }
}

static page_table_descriptor_t *generate_invalid_page_table(void) {

  page_table_descriptor_t *l0_table = malloc(sizeof(page_table_descriptor_t) * 512);

  if(l0_table == NULL) {
    goto err_exit_nalloc;
  }

  if((uint64_t)l0_table & 0xFFF) {
    goto err_exit_l0;
  }

  for(uint_t i = 0; i < 512; i++) {
    l0_table[i].desc_type = PAGE_TABLE_DESC_TYPE_INVALID;
  }

  return l0_table;

err_exit_l0:
  free(l0_table);
err_exit_nalloc:
  return NULL;
}

// Creates a page table with the first 4TB identity mapped to nGnRnE device memory
static page_table_descriptor_t *generate_minimal_page_table(void) {

  page_table_descriptor_t *l0_table = malloc(sizeof(page_table_descriptor_t) * 512);

  if(l0_table == NULL) {
    goto err_exit_nalloc;
  }

  if((uint64_t)l0_table & ((sizeof(page_table_descriptor_t)*512)-1)) {
    PAGING_ERROR("L0 Page Table Allocation was Unaligned!\n");
    goto err_exit_l0;
  }

  for(uint_t i = 1; i < 512; i++) {
    l0_table[i].desc_type = PAGE_TABLE_DESC_TYPE_INVALID;
  }

  // Initialize the first 4TB as device memory
  page_table_descriptor_t *l1_table = malloc(sizeof(page_table_descriptor_t) * 512);

  if(l1_table == NULL) {
    goto err_exit_l0;
  }

  if((uint64_t)l1_table & 0xFFF) {
    goto err_exit_l1;
  }

  l0_table[0].raw = 0;
  l0_table[0].desc_type = PAGE_TABLE_DESC_TYPE_TABLE;
  l0_table[0].raw |= ((uint64_t)l1_table & ~0xFFF);

  for(uint64_t i = 0; i < 512; i++) {
    l1_table[i].desc_type = PAGE_TABLE_DESC_TYPE_BLOCK;
    l1_table[i].mair_index = MAIR_DEVICE_INDEX;
    l1_table[i].access_perm = 0;
    l1_table[i].shareable_attr = OUTER_SHAREABLE;
    l1_table[i].priv_exec_never = 1;
    l1_table[i].unpriv_exec_never = 1;
    l1_table[i].access_flag = 1;
    
    l1_table[i].address_data = 0;
    l1_table[i].raw |= (i*PAGE_SIZE_1GB) & ~0xFFF;
  }

good_exit:
  return l0_table;

err_exit_l1:
  free(l1_table);
err_exit_l0:
  free(l0_table);
err_exit_nalloc:
  return NULL;
}

// Helper function to try and use the largest page sizes possible
inline static void __efficient_drill_helper(uint64_t *start, uint64_t *end, void **drill_func, uint32_t *block_size) {

    if(ALIGNED_1GB(*start) &&
       ALIGNED_1GB(*end)) {

      PAGING_DEBUG("selected page 1GB page size for drill\n");
      *block_size = PAGE_SIZE_1GB;
      *drill_func = drill_page_1gb;

    } else if(ALIGNED_2MB(*start) &&
          ALIGNED_2MB(*end)) {

      PAGING_DEBUG("selected page 2MB page size for drill\n");
      *block_size = PAGE_SIZE_2MB;
      *drill_func = drill_page_2mb;

    } else {

      PAGING_DEBUG("selected page 4KB page size for drill\n");
      // Conservative (shrink the drilled region if needed)
      *end = round_down(*end, PAGE_SIZE_4KB);
      *start = round_up(*start, PAGE_SIZE_4KB);

      *block_size = PAGE_SIZE_4KB;
      *drill_func = drill_page_4kb;

    }
}
   
// Drills pages for the normal memory regions in the map using 1GB pages
static int handle_normal_memory_regions(page_table_descriptor_t *table, void *fdt) {

  int offset = fdt_node_offset_by_prop_value(fdt, -1, "device_type", "memory", 7); 

  while(offset != -FDT_ERR_NOTFOUND) {
    fdt_reg_t reg = {.address = 0, .size = 0};
    int getreg_result = fdt_getreg(fdt, offset, &reg);

    if(getreg_result == 0) {

      uint32_t block_size = PAGE_SIZE_4KB;

      page_table_descriptor_t*(*drill_func)(page_table_descriptor_t*, void*) = NULL;
   
      uint64_t start = reg.address;
      uint64_t end = reg.address + reg.size;

      __efficient_drill_helper(&start, &end, &drill_func, &block_size);

      for(; start < end; start += block_size) {
        page_table_descriptor_t *desc;
        desc = (*drill_func)(table, (void*)start);

        if(desc == NULL) {
          PAGING_WARN("Failed to drill page for normal memory!\n");
        }

        desc->mair_index = MAIR_NORMAL_INDEX;
        desc->access_perm = 0;
#if NAUT_CONFIG_MAX_CPUS == 1
        desc->shareable_attr = NON_SHAREABLE;
#else
        desc->shareable_attr = INNER_SHAREABLE;
#endif
        desc->priv_exec_never = 1;
        desc->unpriv_exec_never = 1;

        desc->access_flag = 1; 

        // We assume the entry is still an identity mapping
      }
    }

    offset = fdt_node_offset_by_prop_value(fdt, offset, "device_type", "memory", 7);
  }

  return 0;
}

static int handle_text_memory_regions(page_table_descriptor_t *table) {
  extern int _textStart;
  extern int _textEnd;

  uint64_t start = &_textStart;
  uint64_t end = &_textEnd;

  uint32_t block_size = PAGE_SIZE_4KB;

  page_table_descriptor_t*(*drill_func)(page_table_descriptor_t*, void*) = NULL;  

  __efficient_drill_helper(&start, &end, &drill_func, &block_size);

  for(; start < end; start += block_size) {
    
    page_table_descriptor_t *desc;
    desc = (*drill_func)(table, (void*)start);

    if(desc == NULL) {
      // An actual error not just a warning
      PAGING_ERROR("Failed to drill page for text memory!\n");
      return -1;
    }

    desc->priv_exec_never = 0;
    desc->unpriv_exec_never = 0;

    // Don't modify identity mapping
  }

  return 0;

}

static int handle_rodata_memory_regions(page_table_descriptor_t *table) {
  extern int _rodataStart;
  extern int _rodataEnd;

  uint64_t start = &_rodataStart;
  uint64_t end = &_rodataEnd;

  uint32_t block_size = PAGE_SIZE_4KB;

  page_table_descriptor_t*(*drill_func)(page_table_descriptor_t*, void*) = NULL;  

  __efficient_drill_helper(&start, &end, &drill_func, &block_size);

  for(; start < end; start += block_size) {
    
    page_table_descriptor_t *desc;
    desc = (*drill_func)(table, (void*)start);

    if(desc == NULL) {
      // An actual error not just a warning
      PAGING_ERROR("Failed to drill page for text memory!\n");
      return -1;
    }

    // Read only
    desc->access_perm |= 0b10;

    // Don't modify identity mapping
  }

  return 0;

}


static page_table_descriptor_t *low_mem_table;
static page_table_descriptor_t *high_mem_table;

int arch_paging_init(struct nk_mem_info *mm_info, void *fdt) {
 
  mair_reg_t mair;
  LOAD_SYS_REG(MAIR_EL1, mair.raw);
  mair.attrs[MAIR_DEVICE_INDEX] = MAIR_ATTR_DEVICE_nGnRnE;
  mair.attrs[MAIR_NORMAL_INDEX] = MAIR_ATTR_NORMAL_INNER_OUTER_WB_CACHEABLE;
  STORE_SYS_REG(MAIR_EL1, mair.raw);

  PAGING_DEBUG("Configured MAIR register: MAIR_EL1 = 0x%x\n", mair.raw);

  low_mem_table = generate_minimal_page_table();

  if(low_mem_table == NULL) {
    PAGING_ERROR("Could not create low mem page table!\n");
    return -1;
  }

  if(handle_normal_memory_regions(low_mem_table, fdt)) {
    PAGING_WARN("Failed to handle normal page regions! (All memory is device memory)\n");
  }

  if(handle_text_memory_regions(low_mem_table)) {
    PAGING_ERROR("Failed to handle text page regions! (GOING TO CRASH IF WE CONTINUE ENABLING PAGING)\n");
    return -1;
  }

  if(handle_rodata_memory_regions(low_mem_table)) {
    PAGING_WARN("Failed to handle rodata page regions! (Read only sections may be modified without an exception!)\n");
    return -1;
  }

  high_mem_table = generate_invalid_page_table();

  if(high_mem_table == NULL) {
    PAGING_ERROR("Could not create high mem page table!\n");
    return -1;
  }

  return 0;
}

int per_cpu_paging_init(void) {

  STORE_SYS_REG(TTBR0_EL1, (uint64_t)low_mem_table);
  STORE_SYS_REG(TTBR1_EL1, (uint64_t)high_mem_table);

  PAGING_DEBUG("Set Low and High Mem Page Tables: TTBR0_EL1 = 0x%x, TTBR1_EL1 = 0x%x\n", (uint64_t)low_mem_table, (uint64_t)high_mem_table);

  tc_reg_t tcr;

  LOAD_SYS_REG(TCR_EL1, tcr.raw);

  tcr.low_mem_msb_mask = 16;
  tcr.high_mem_msb_mask = 16;

  tcr.ttbr0_do_not_walk = 0;
  tcr.ttbr0_granule_size = TCR_TTBR0_GRANULE_SIZE_4KB;
  tcr.ttbr0_top_bit_ignored = 0;
  tcr.ttbr0_table_inner_cacheability = TCR_DEFAULT_CACHEABILITY;
  tcr.ttbr0_table_outer_cacheability = TCR_DEFAULT_CACHEABILITY;
  tcr.ttbr0_table_shareability = TCR_DEFAULT_SHAREABILITY;
  tcr.ttbr0_disable_hierarchy = 1;
  
  tcr.ttbr1_do_not_walk = 0;
  tcr.ttbr1_granule_size = TCR_TTBR1_GRANULE_SIZE_4KB;
  tcr.ttbr1_top_bit_ignored = 0;
  tcr.ttbr1_table_inner_cacheability = TCR_DEFAULT_CACHEABILITY;
  tcr.ttbr1_table_outer_cacheability = TCR_DEFAULT_CACHEABILITY;
  tcr.ttbr1_table_shareability = TCR_DEFAULT_SHAREABILITY;
  tcr.ttbr1_disable_hierarchy = 1;

  tcr.high_mem_defines_asid = 0;
  tcr.asid_16bit = 0;
  tcr.ips_size = 0b101; // 48 bits physical
  tcr.hw_update_acc_flag = 0;
  tcr.hw_update_dirty_bit = 0;

#ifdef NAUT_CONFIG_DEBUG_PAGING
  dump_tc_reg(tcr);
#endif

  STORE_SYS_REG(TCR_EL1, tcr.raw);

#ifdef NAUT_CONFIG_DEBUG_PAGING
  dump_page_table(low_mem_table);
#endif

  PAGING_DEBUG("Enabling the MMU\n");

  sys_ctrl_reg_t ctl;

  LOAD_SYS_REG(SCTLR_EL1, ctl.raw);

  ctl.instr_cacheability_ctrl = 1;

  STORE_SYS_REG(SCTLR_EL1, ctl.raw);

  asm volatile ("isb");

  ctl.mmu_en = 1;
  ctl.write_exec_never = 0;

  extern void __mmu_enable(uint64_t sctlr);
  __mmu_enable(ctl.raw);

  PAGING_DEBUG("MMU Enabled\n");

  return 0;
}
