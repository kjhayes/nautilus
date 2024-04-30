#ifndef __NAUTILUS_INIT_STAGES_H__
#define __NAUTILUS_INIT_STAGES_H__

/*
 * The order of this list determines the order each stage occurs
 * during kernel initialization
 *
 * Adding a stage to the list automatically generates:
 *     - The "nk_handle_init_stage_STAGE(void)" function
 *       (it still needs to be called manually from arch/.../init.c)
 *     - The ".init.STAGE" linker section (and subsections)
 *     - "nk_decl_init_stage_func(FUNC, STAGE)" for the stage
 *        (a helper function "nk_decl_STAGE_init(FUNC)" needs to be added explicitly to "nautilus/init.h")
 */
#define NK_INIT_STAGES_LIST \
    X(silent)\
    X(static)\
    X(boot)\
    X(kmem)\
    X(sched)\
    X(subsys)\
    X(driver)\
    X(fs)\
    X(launch)\

/*
 * silent -> no printk
 * static -> no memory allocator
 * boot -> boot memory allocator
 * kmem -> kmem allocator
 * sched -> threads and scheduling are enabled from here on
 * subsys -> kernel subsystems (pci, cxx support, instrumentation)
 * driver -> specific device drivers
 * fs -> filesystem initialization
 * launch -> launch threads, mount filesystems, etc.
 */

#endif
