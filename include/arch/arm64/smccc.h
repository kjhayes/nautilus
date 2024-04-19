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

#ifndef __ARCH64_SMCCC_H__
#define __ARCH64_SMCCC_H__

#define SMCCC_CALL_TYPE_ARCH 0

#define SMCCC_ARCH_RET_SUCCESS       ( 0)
#define SMCCC_ARCH_RET_NOT_SUPPORTED (-1)
#define SMCCC_ARCH_RET_NOT_REQUIRED  (-2)
#define SMCCC_ARCH_RET_INVALID_PARAM (-3)

#define SMCCC_CALL_TYPE_CPU_SERV 1
#define SMCCC_CALL_TYPE_SIP_SERV 2
#define SMCCC_CALL_TYPE_OEM_SERV 3
#define SMCCC_CALL_TYPE_SS_SERV 4
#define SMCCC_CALL_TYPE_SH_SERV 5
#define SMCCC_CALL_TYPE_VSH_SERV 6

void smc64_call(
    uint16_t id,
    uint8_t type,
    uint64_t *args,
    uint64_t arg_count,
    uint64_t *rets, 
    uint64_t ret_count
    );

void hvc64_call(
    uint16_t id,
    uint8_t type,
    uint64_t *args,
    uint64_t arg_count,
    uint64_t *rets, 
    uint64_t ret_count
    );

void smc32_call(
    uint16_t id,
    uint8_t type,
    uint32_t *args,
    uint64_t arg_count,
    uint32_t *rets, 
    uint64_t ret_count
    );

void hvc32_call(
    uint16_t id,
    uint8_t type,
    uint32_t *args,
    uint64_t arg_count,
    uint32_t *rets, 
    uint64_t ret_count
    );

#endif
