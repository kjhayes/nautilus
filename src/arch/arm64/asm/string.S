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

.section .text

.global nk_low_level_memset

# x0 = void *dest
# x1 = char data
# x2 = size_t count

.align 3
nk_low_level_memset:

# Calculate end
  add x2, x2, x0 

1:
  cmp x2, x0
  b.eq 2f

  strb w1, [x0]
  add x0, x0, 1

  b 1b
2:

  ret

.global nk_low_level_memcpy

# x0 = void *dest
# x1 = char *src
# x2 = size_t count

.align 3
nk_low_level_memcpy:
  // TODO(arm64)
  b .
  ret
