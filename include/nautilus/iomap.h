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

#ifndef __IOMAP_H__
#define __IOMAP_H__

#include <nautilus/naut_types.h>
#include <nautilus/rbtree.h>

#define NK_IO_MAPPING_FLAG_ENABLED (1<<0)

struct nk_io_mapping {
  void *paddr;
  void *vaddr;
  size_t size;

  struct rb_node node;

  int flags;
  int alloc;
};

void * nk_io_map(void *paddr, size_t size, int flags);
int nk_io_unmap(void *paddr);

struct nk_io_mapping *nk_io_map_first(void);
struct nk_io_mapping *nk_io_map_next(struct nk_io_mapping *iter);

void dump_io_map(void);

// KJH - Theoretically these functions could need to be 
// architecture dependant, so use these to access
// mmio regions instead of direct access where possible

static inline uint8_t readb(uint64_t addr) {
  return *(volatile uint8_t*)addr;
}
static inline uint16_t readw(uint64_t addr) {
  return *(volatile uint16_t*)addr;
}
static inline uint32_t readl(uint64_t addr) {
  return *(volatile uint32_t*)addr;
}
static inline uint64_t readq(uint64_t addr) {
  return *(volatile uint64_t*)addr;
}

static inline void writeb(uint8_t val, uint64_t addr) {
  *(volatile uint8_t*)addr = val;
}
static inline void writew(uint16_t val, uint64_t addr) {
  *(volatile uint16_t*)addr = val;
}
static inline void writel(uint32_t val, uint64_t addr) {
  *(volatile uint32_t*)addr = val;
}
static inline void writeq(uint64_t val, uint64_t addr) {
  *(volatile uint64_t*)addr = val;
}

#endif
