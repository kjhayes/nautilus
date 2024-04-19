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
 * Copyright (c) 2023, Kirill Nagaitsev <knagaitsev@u.northwestern.edu>
 *                     Kevin Hayes <kjhayes@u.northwestern.edu>
 * Copyright (c) 2023, The V3VEE Project  <http://www.v3vee.org>
 *                     The Hobbes Project <http://xstack.sandia.gov/hobbes>
 * All rights reserved.
 *
 * Authors: Kirill Nagaitsev <knagaitsev@u.northwestern.edu>
 *          Kevin Hayes <kjhayes@u.northwestern.edu>
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "LICENSE.txt".
 */

#ifndef __NAUT_FDT_H__
#define __NAUT_FDT_H__

#include<lib/libfdt.h>

/*
 * Nautilus' extensions to libfdt
 */

typedef struct fdt_reg {
	off_t address;
	size_t size;
} fdt_reg_t;

// returns 0 on success, anything else on failure
int fdt_getreg(const void *fdt, int offset, fdt_reg_t *reg);
int fdt_getreg_array(const void *fdt, int offset, fdt_reg_t *reg, int *num);

// NULL on failure (not ideal for the zero CPU)
off_t fdt_getreg_address(const void *fdt, int offset);

void print_fdt(const void *fdt);

// callback returns 0 if walking should stop, 1 otherwise
void fdt_walk_devices(const void *fdt, int (*callback)(const void *fdt, int offset, int depth));

int fdt_nodename_eq(const void *fdt, int offset, const char *s, int len);

#endif
