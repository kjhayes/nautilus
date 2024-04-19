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

#ifndef __RADIX_TREE_H__
#define __RADIX_TREE_H__

#include<nautilus/naut_types.h>

struct nk_radix_tree_layer;

// Synchronization is the responsibility of the user
struct nk_radix_tree 
{
  int height;
  struct nk_radix_tree_layer *root;
};

struct nk_radix_tree_iterator 
{
  unsigned long index;
  int height;
  struct nk_radix_tree_layer *layer;
};

void *nk_radix_tree_get(struct nk_radix_tree *tree, unsigned long index);
void *nk_radix_tree_remove(struct nk_radix_tree *tree, unsigned long index);
int nk_radix_tree_insert(struct nk_radix_tree *tree, unsigned long index, void *data);

int nk_radix_tree_iterator_start(struct nk_radix_tree *tree, struct nk_radix_tree_iterator *iter);
int nk_radix_tree_iterator_next(struct nk_radix_tree_iterator *iter);
int nk_radix_tree_iterator_check_end(struct nk_radix_tree_iterator *iter);

unsigned long nk_radix_tree_iterator_index(struct nk_radix_tree_iterator *iter);
void *nk_radix_tree_iterator_value(struct nk_radix_tree_iterator *iter);

#endif
