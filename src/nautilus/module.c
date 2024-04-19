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
#include <nautilus/nautilus.h>
#include <nautilus/mb_utils.h>
#include <nautilus/list.h>
#include <nautilus/module.h>

#ifndef NAUT_CONFIG_DEBUG_MODULES
#undef DEBUG_PRINT
#define DEBUG_PRINT(fmt, args...) 
#endif

#define INFO(fmt, args...)  INFO_PRINT("Module: " fmt, ##args)
#define DEBUG(fmt, args...) DEBUG_PRINT("Module: " fmt, ##args)
#define WARN(fmt, args...)  WARN_PRINT("Module: " fmt, ##args)
#define ERROR(fmt, args...) ERROR_PRINT("Module: " fmt, ##args)



int nk_init_builtin_modules(void) 
{
    extern struct nk_module * __start_modules[];
    extern struct nk_module * __stop_modules[];
    size_t modules_size = (void*)__stop_modules - (void*)__start_modules;
    size_t num_modules = modules_size / sizeof(struct nk_module*);

    DEBUG(".modules size in bytes = %lu\n", modules_size);
    DEBUG(".modules num entries = %lu\n", num_modules);

    int num_failures = 0;
    for(size_t i = 0; i < num_modules; i++) {
        struct nk_module *mod = __start_modules[i];
        INFO("Trying to intialize built-in module: \"%s\"\n", mod->name);
        if(nk_try_init_module(mod)) {
          ERROR("Failed to initialize built-in module: \"%s\"!\n", mod->name);
          num_failures += 1;
        } else {
          INFO("Initialized built-in module: \"%s\"\n", mod->name);
        }
    }
    return num_failures;
}

int nk_try_init_module(struct nk_module *mod) 
{
    if(mod->flags & NK_MOD_FLAG_INITED) {
        // This module has already been initialized
        return 0;
    }

    int ret = (mod->init)();
    if(ret) {
        return ret;
    } else {
        mod->flags |= NK_MOD_FLAG_INITED;
        return 0;
    }
}

#ifdef NAUT_CONFIG_USE_MULTIBOOT
/*
 * The current model of modules is that they are basically binary blobs, but
 * can be used for different purposes. They are currently Multiboot2 specific.
 * Currently, the module just gets inserted into a list, which later subsystems
 * can traverse to find modules they care about. 
 *
 * This will eventually turn into a callback-oriented interface (think,
 * pci_probe). 
 *
 * There are initially only two types of modules, a symbol table (much like
 * System.map in Linux) and an ELF module, which is a PIC-compiled shared
 * object which will be run as a program.
 */


int
nk_register_mod (struct multiboot_info * mb_info, struct multiboot_tag_module * m)
{
    /* We use the boot allocator as this is going to run very early on */
    struct multiboot_mod * mod = mm_boot_alloc(sizeof(struct multiboot_mod));
    uint32_t * cursor; 

    if (!mod) {
        ERROR_PRINT("Could not allocate multiboot module\n");
        return -1;
    }

    memset(mod, 0, sizeof(struct multiboot_mod));

    mod->start   = m->mod_start;
    mod->end     = m->mod_end;
    mod->cmdline = m->cmdline;

    cursor = (uint32_t*)mod->start;

    switch (*cursor) {
        case ST_MAGIC: 
            DEBUG_PRINT("Found symbol table module\n");
            mod->type = MOD_SYMTAB;
            break;
        case ELF_MAGIC:
            DEBUG_PRINT("Found ELF program module\n");
            mod->type = MOD_PROGRAM;
            break;
        default:
            ERROR_PRINT("Found unknown module (magic=0x%08x)\n", *cursor);
    }

    list_add(&mod->elm, &(mb_info->mod_list));

    return 0;
}
#endif
