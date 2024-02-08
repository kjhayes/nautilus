
#include <nautilus/topo.h>
#include <nautilus/nautilus.h>
#include <nautilus/shell.h>
#include <nautilus/naut_types.h>

uint32_t
nk_topo_get_smt_id (struct cpu * cpu)
{
    return cpu->coord->smt_id;
}


uint32_t
nk_topo_get_my_smt_id (void)
{
    return nk_topo_get_smt_id(get_cpu());
}


uint32_t
nk_topo_get_socket_id (struct cpu * cpu)
{
    return cpu->coord->pkg_id;
}


uint32_t
nk_topo_get_my_socket_id (void)
{
    return nk_topo_get_socket_id(get_cpu());
}

uint32_t
nk_topo_get_phys_core_id (struct cpu * cpu)
{
    return cpu->coord->core_id;
}

uint32_t
nk_topo_get_my_phys_core_id (void)
{
    return nk_topo_get_phys_core_id(get_cpu());
}

uint8_t
nk_topo_cpus_share_phys_core (struct cpu * a, struct cpu * b)
{
    return nk_topo_cpus_share_socket(a, b) && (a->coord->core_id == b->coord->core_id);
}

uint8_t
nk_topo_same_phys_core_as_me (struct cpu * other)
{
    return nk_topo_cpus_share_phys_core(get_cpu(), other);
}


uint8_t
nk_topo_cpus_share_socket (struct cpu * a, struct cpu * b)
{
    return a->coord->pkg_id == b->coord->pkg_id;
}

uint8_t
nk_topo_same_socket_as_me (struct cpu * other)
{
    return nk_topo_cpus_share_socket(get_cpu(), other);
}

static uint8_t (*const cpu_filter_funcs[])(struct cpu*, struct cpu*) = 
{
    [NK_TOPO_PHYS_CORE_FILT] = nk_topo_cpus_share_phys_core,
    [NK_TOPO_SOCKET_FILT]    = nk_topo_cpus_share_socket,
};


// We assume CPUs are not changing at this point, so no locks necessary
void
nk_topo_map_sibling_cpus (void (func)(struct cpu * cpu, void * state), nk_topo_filt_t filter, void * state)
{
    struct sys_info * sys = per_cpu_get(system);
    int i;

    if (filter != NK_TOPO_PHYS_CORE_FILT && filter != NK_TOPO_SOCKET_FILT && filter != NK_TOPO_ALL_FILT) {
        ERROR_PRINT("Sibling CPU mapping only supports socket-level and physcore-level mapping\n");
        return;
    }

    for (i = 0; i < nk_get_num_cpus(); i++) {
		if (i == my_cpu_id())
			continue;

        if (filter == NK_TOPO_ALL_FILT || cpu_filter_funcs[filter](get_cpu(), sys->cpus[i])) 
            func(sys->cpus[i], state);
    }
}

void 
nk_topo_map_core_sibling_cpus (void (func)(struct cpu * cpu, void * state), void * state)
{
    nk_topo_map_sibling_cpus(func, NK_TOPO_PHYS_CORE_FILT, state);
}

void 
nk_topo_map_socket_sibling_cpus (void (func)(struct cpu * cpu, void * state), void * state)
{
    nk_topo_map_sibling_cpus(func, NK_TOPO_SOCKET_FILT, state);
}

static void
topo_test (struct cpu * cpu, void * state)
{
    nk_vc_printf("MAPPER: Applying func to CPU %d\n", cpu->id);
}

static int
handle_cputopotest (char * buf, void * priv)
{
    char aps;
    if (((aps='a', strcmp(buf,"cputopotest a"))==0) ||
			((aps='p', strcmp(buf,"cputopotest p"))==0) ||
			((aps='s', strcmp(buf,"cputopotest s"))==0)) {
        switch (aps) { 
            case 'a': 
				nk_vc_printf("Mapping func to all siblings\n");
                nk_topo_map_sibling_cpus(topo_test, NK_TOPO_ALL_FILT, NULL);
                break;
            case 'p': 
				nk_vc_printf("Mapping func to core siblings\n");
                nk_topo_map_core_sibling_cpus(topo_test, NULL);
                break;
            case 's': 
				nk_vc_printf("Mapping func to socket siblings\n");
                nk_topo_map_socket_sibling_cpus(topo_test, NULL);
                break;
            default:
                nk_vc_printf("Unknown cputopotest command requested\n");
                return -1;
        }
    }

    return 0;
}

static struct shell_cmd_impl cputopo_impl = {
    .cmd      = "cputopotest",
    .help_str = "cputopotest <a|p|s>",
    .handler  = handle_cputopotest,
};
nk_register_shell_cmd(cputopo_impl);


