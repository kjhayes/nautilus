/* 
 * This file is part of the Nautilus AeroKernel developed
 * by the Hobbes and V3VEE Projects with funding from the 
 * United States National  Science Foundation and the Department of Energy.  
 *
 * The V3VEE Project is a joint project between Northwestern University
 * and the University of New Mexico.  The Hobbes Project is a collaboration
 * led by Sandia National Laboratories that includes several national 
 * laboratories and universities. You can find out more at:
 * http://www.v3vee.org  and
 * http://xstack.sandia.gov/hobbes
 *
 * Copyright (c) 2018, Kyle C. Hale <khale@cs.iit.edu>
 * Copyright (c) 2018, The V3VEE Project  <http://www.v3vee.org> 
 *                     The Hobbes Project <http://xstack.sandia.gov/hobbes>
 * All rights reserved.
 *
 * Author: Kyle C. Hale <khale@cs.iit.edu>
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "LICENSE.txt".
 */
#ifndef __MODULE_H__
#define __MODULE_H__

#include <nautilus/nautilus.h>

#define NK_MOD_FLAG_INITED    (1<<0)

struct nk_module
{
    const char *name;
    int(*init)(void);
    int flags;
};

// Returns the number of modules which returned an error code
int nk_init_builtin_modules(void);

// Zero if the module is initialized on return, non-zero otherwise
int nk_try_init_module(struct nk_module *info);

#define nk_declare_module(MOD) \
    __attribute__((used, section(".module." # MOD))) \
    static struct nk_module *__nk_mod_##MOD = &MOD;

// Multiboot Module Stuff
#ifdef NAUT_CONFIG_USE_MULTIBOOT
#define ST_MAGIC    0x00952700  // magic number of symbol table file
#define ELF_MAGIC   0x464c457f

int nk_register_mod(struct multiboot_info * mb_info, struct multiboot_tag_module * m);
#endif

#endif
