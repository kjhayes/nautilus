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

#ifndef __ARM64_PSCI_H__
#define __ARM64_PSCI_H__

#include<nautilus/naut_types.h>

#define PSCI_SUCCESS 0
#define PSCI_NOT_SUPPORTED (-1)
#define PSCI_INVALID_PARAMETERS (-2)
#define PSCI_DENIED (-3)
#define PSCI_ALREADY_ON (-4)
#define PSCI_ON_PENDING (-5)
#define PSCI_INTERNAL_FAILURE (-6)
#define PSCI_NOT_PRESENT (-7)
#define PSCI_DISABLED (-8)
#define PSCI_INVALID_ADDRESS (-9)

int psci_init(void *dtb);
int psci_version(uint16_t *major, uint16_t *minor);
int psci_cpu_on(uint64_t target_mpid, void *entry_point, uint64_t context_id);

/*
#define PSCI_AFFINITY_INFO_ON 0
#define PSCI_AFFINITY_INFO_OFF 1
#define PSCI_AFFINITY_INFO_ON_PENDING 2
// INVALID_PARAMETERS and DISABLED are also valid return values from this function
int psci_affinity_info(uint64_t target_mpid);
*/

// These functions can fail, so make sure the programmer doesn't ignore that possibility
int psci_cpu_off(void);
int psci_system_off(void);
int psci_system_reset(void);

#endif
