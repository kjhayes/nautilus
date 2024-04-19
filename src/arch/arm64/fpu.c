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


#include<arch/arm64/fpu.h>

#include<arch/arm64/sys_reg.h>

int fpu_init_percpu(struct naut_info *info) {

  cpacr_el1_t cpacr;
  LOAD_SYS_REG(CPACR_EL1, cpacr.raw);

  cpacr.fp_enabled = 1;
  cpacr.sve_enabled = 1;

  STORE_SYS_REG(CPACR_EL1, cpacr.raw);

  asm volatile ("isb");

  fpcr_t fpcr;
  LOAD_SYS_REG(FPCR, fpcr.raw);

  // Here we can modify the fpu settings
  fpcr.raw = 0;
  fpcr.raw |= (1<<8); // Invalid Operation Exception
  fpcr.raw |= (1<<9); // Divide by Zero Exception
  fpcr.raw |= (1<<10); // Overflow Exception
  fpcr.raw |= (1<<11); // Underflow Exception
  fpcr.raw |= (1<<12); // Inexact Exception

  STORE_SYS_REG(FPCR, fpcr.raw);

  return 0;
}

