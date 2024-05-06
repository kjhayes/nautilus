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

.global init_smp_boot

init_smp_boot:
  // TODO
  ret

.align 3
.global end_smp_boot
end_smp_boot:


.global smp_ap_stack_switch

// x0 := new stack pointer
// x1 := new base pointer
// x2 := cpu struct ptr
// returns the new cpu struct ptr
// ARM likes to put some strange things on the stack,
// so we're going to put in some extra effort to copy the bootstack
// This means any stack we allocate for a thread, needs to be larger than
// the bootstack (or atleast as large as the space the bootstack uses)
.extern boot_stack_end
.global smp_ap_stack_switch
smp_ap_stack_switch:

  mov x3, sp

  ldr x4, =boot_stack_end

1:
  cmp x4, x3
  ble 2f

  ldrb w5, [x4]
  strb w5, [x0]
  sub x0, x0, 1
  sub x4, x4, 1

  b 1b
2:

  mov x0, x2
  ret

