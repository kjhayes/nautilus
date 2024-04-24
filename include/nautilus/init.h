#ifndef __NAUTILUS_INIT_H__
#define __NAUTILUS_INIT_H__

#include <nautilus/init_stages.h>

#ifndef __LINKER__
#include <nautilus/errno.h>
#endif

typedef int(*nk_init_func_t)(void);

// Priority here shouldn't be semantic, it should just help avoid unnecessary deferrals in some cases
#define nk_declare_init_stage_func(FUNC, STAGE, PRIORITY) \
    __attribute__((used, section(".init." #STAGE "." #PRIORITY "." #FUNC)))\
    static nk_init_func_t __init_##STAGE##_##FUNC = FUNC

// Helper Macros, (can't be automatically generated from NK_INIT_STAGES_LIST :( )
#define nk_declare_arch_init(FUNC)   nk_declare_init_stage_func(FUNC, arch, 10)
#define nk_declare_silent_init(FUNC)   nk_declare_init_stage_func(FUNC, silent, 10)
#define nk_declare_static_init(FUNC) nk_declare_init_stage_func(FUNC, static, 10)
#define nk_declare_boot_init(FUNC)   nk_declare_init_stage_func(FUNC, boot, 10)
#define nk_declare_kmem_init(FUNC)   nk_declare_init_stage_func(FUNC, kmem, 10)
#define nk_declare_sched_init(FUNC)   nk_declare_init_stage_func(FUNC, sched, 10)
#define nk_declare_subsys_init(FUNC) nk_declare_init_stage_func(FUNC, subsys, 10)
#define nk_declare_driver_init(FUNC) nk_declare_init_stage_func(FUNC, driver, 10)

#define X(STAGE)\
int nk_handle_init_stage_##STAGE(void);
NK_INIT_STAGES_LIST
#undef X

#endif
