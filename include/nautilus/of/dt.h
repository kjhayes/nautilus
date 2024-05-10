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

#ifndef __NAUT_DEVICE_TREE_H__
#define __NAUT_DEVICE_TREE_H__

#include<nautilus/dev.h>
#include<nautilus/dev_info.h>

#define DT_NODE_FLAG_INT_PARENT_PHANDLE  (1<<0)
#define DT_NODE_FLAG_CACHED_IRQ_DATA     (1<<1)
#define DT_NODE_FLAG_CACHED_IRQ_EXT_DATA (1<<2)

struct dt_node;
struct dt_node {
  struct nk_dev_info dev_info;
  int flags;

  uint32_t phandle;

  // Cached IRQ (EXT) data
  int num_irq;
  int irq_cells;
  int num_irq_extended;
  uint32_t *extended_parent_phandles;
  int *extended_offsets;

  struct dt_node *parent;
  struct dt_node *siblings;
  struct dt_node *children;
  int child_count;

  union {
    uint32_t interrupt_parent_phandle;
    struct dt_node *interrupt_parent;
  };

  void *dtb;
  int dtb_offset;

  struct dt_node *next_node;
};

struct of_dev_properties {
  const int count;
  const char **names;
};

struct of_dev_id {
  const char *name;
  const char *compatible;
  const struct of_dev_properties *properties;
};

struct of_dev_match {
  const struct of_dev_id * ids;
  const int num_ids;

  const int max_num_matches;
};

#define declare_of_dev_match_id_array(VAR, ID_ARR) \
  static struct of_dev_match VAR = { \
    .ids = ID_ARR, \
    .num_ids = sizeof(ID_ARR)/sizeof(struct of_dev_id) \
  };

int fdt_free_tree(struct dt_node *root);
int fdt_unflatten_tree(void *fdt, struct dt_node **root);


// Returns the number of devices which matched but init_callback returned NULL
int of_for_each_match(const struct of_dev_match *match, int(*callback)(struct nk_dev_info *info));
int of_for_each_subnode_match(const struct of_dev_match *match, int(*callback)(struct nk_dev_info *info), struct nk_dev_info *root);

#endif
