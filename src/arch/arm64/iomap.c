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


#include<nautilus/iomap.h>
#include<nautilus/arch.h>
#include<nautilus/atomic.h>

#include<arch/arm64/paging.h>

int arch_handle_io_map(struct nk_io_mapping *mapping) 
{
  if(mapping->flags & NK_IO_MAPPING_FLAG_ENABLED) {
    return 0;
  }

  if(ttbr0_table == NULL) {
    return 0;
  } else {
    int flags = atomic_or(mapping->flags, NK_IO_MAPPING_FLAG_ENABLED);
    return pt_init_table_device(ttbr0_table, (uint64_t)mapping->vaddr, (uint64_t)mapping->paddr, mapping->size);
  }
}
int arch_handle_io_unmap(struct nk_io_mapping *mapping) 
{
  // TODO
  return -1;
}
