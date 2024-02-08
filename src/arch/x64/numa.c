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
 * http://xtack.sandia.gov/hobbes
 *
 * Copyright (c) 2015, Kyle C. Hale <kh@u.northwestern.edu>
 * Copyright (c) 2015, The V3VEE Project  <http://www.v3vee.org> 
 *                     The Hobbes Project <http://xstack.sandia.gov/hobbes>
 * All rights reserved.
 *
 * Author: Kyle C. Hale <kh@u.northwestern.edu>
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "LICENSE.txt".
 */
#include <nautilus/nautilus.h>
#include <nautilus/numa.h>
#include <nautilus/mm.h>
#include <nautilus/macros.h>
#include <nautilus/errno.h>
#include <nautilus/acpi.h>
#include <nautilus/math.h>

#include <arch/x64/cpuid.h>

#define u8  uint8_t
#define u16 uint16_t
#define u32 uint32_t
#define u64 uint64_t

static int srat_rev;


#ifndef NAUT_CONFIG_DEBUG_NUMA
#undef DEBUG_PRINT
#define DEBUG_PRINT(fmt, args...)
#endif
#define NUMA_PRINT(fmt, args...) printk("NUMA: " fmt, ##args)
#define NUMA_DEBUG(fmt, args...) DEBUG_PRINT("NUMA: " fmt, ##args)
#define NUMA_ERROR(fmt, args...) ERROR_PRINT("NUMA: " fmt, ##args)
#define NUMA_WARN(fmt, args...)  WARN_PRINT("NUMA: " fmt, ##args)


static inline uint32_t 
next_pow2 (uint32_t v)
{
    v--;
    v |= v>>1;
    v |= v>>2;
    v |= v>>4;
    v |= v>>8;
    return ++v;
}

/* logical processor count */
static uint8_t
get_max_id_per_pkg (void)
{
    cpuid_ret_t ret;
    cpuid(1, &ret);
    return ((ret.b >> 16) & 0xff);
}


static uint8_t
amd_get_cmplegacy (void)
{
    cpuid_ret_t ret;
    cpuid(CPUID_AMD_FEATURE_INFO, &ret);
    return ret.c & 0x2;
}

static uint8_t
has_leafb (void)
{
    cpuid_ret_t ret;
    cpuid_sub(11, 0, &ret);
    return (ret.b != 0);
}

static uint8_t 
has_htt (void)
{
    cpuid_ret_t ret;
    cpuid(1, &ret);
    return !!(ret.d & (1<<28));
}


static void
intel_probe_with_leafb (struct nk_topo_params *tp)
{
	cpuid_ret_t ret;
	unsigned subleaf = 0;
	unsigned level_width = 0;
	uint8_t level_type = 0;
	uint8_t found_core = 0;
	uint8_t found_smt = 0;

	// iterate through the 0xb subleafs until EBX == 0
	do {
		cpuid_sub(0xb, subleaf, &ret);
		
		// this is an invalid subleaf
		if ((ret.b & 0xffff) == 0) 
			break;

		level_type  = (ret.c >> 8) & 0xffff;
		level_width = ret.a & 0xf;

		switch (level_type) {
			case 0x1: // level type is SMT, which means level_width holds the SMT mask width
				NUMA_DEBUG("Found smt mask width: %d\n", level_width);
				tp->smt_mask_width = level_width;
				tp->smt_select_mask = ~(0xffffffff << tp->smt_mask_width);
				found_smt = 1;
				break;
			case 0x2: // level type is SMT + core, which means level_width holds the width of both
				NUMA_DEBUG("Found smt + core mask width: %d\n", level_width);
				found_core = 1;
				break;
			default:
				break;
		}
		subleaf++;
	} while (1);

	tp->core_plus_mask_width = level_width;
	tp->core_plus_select_mask = ~(0xffffffff << tp->core_plus_mask_width);
	tp->pkg_select_mask_width = level_width;
	tp->pkg_select_mask = 0xffffffff ^ tp->core_plus_select_mask;

	if (found_smt && found_core)
		tp->core_only_select_mask = tp->core_plus_select_mask ^ tp->smt_select_mask;
	else 
		NUMA_WARN("Strange Intel hardware topology detected\n");
}

static void
intel_probe_with_leaves14 (struct nk_topo_params * tp)
{
    cpuid_ret_t ret;
    if (cpuid_leaf_max() >= 0x4) {
        NUMA_DEBUG("Intel probing topo with leaves 1 and 4\n");
        cpuid(1, &ret);
        uint32_t first = next_pow2((ret.b >> 16) & 0xff);
        cpuid_sub(4, 0, &ret);
        uint32_t second = ((ret.a >> 26) & 0xfff) + 1;
        tp->smt_mask_width = ilog2(first/second);
        tp->smt_select_mask = ~(0xffffffff << tp->smt_mask_width);
        uint32_t core_only_mask_width = ilog2(second);
        tp->core_only_select_mask = (~(0xffffffff << (core_only_mask_width + tp->smt_mask_width))) ^ tp->smt_select_mask;
        tp->core_plus_mask_width = core_only_mask_width + tp->smt_mask_width;
        tp->pkg_select_mask = 0xffffffff << tp->core_plus_mask_width;
    } else { 
        NUMA_ERROR("SHOULDN'T GET HERE!\n");
    }
}

static void
amd_get_topo_legacy (struct nk_topo_params * tp)
{
    cpuid_ret_t ret;
    uint32_t coreidsize;

    cpuid(0x80000008, &ret);

    coreidsize = (ret.c >> 12) & 0xf;

    if (!coreidsize) 
        tp->max_ncores = (ret.c & 0xff) + 1;
    else
        tp->max_ncores = 1 << coreidsize;

    tp->max_nthreads = 1;
}


static void
amd_get_topo_secondary (struct nk_topo_params * tp)
{
    cpuid_ret_t ret;
    uint32_t largest_extid;
    uint8_t has_x2;

    NUMA_DEBUG("Attempting AMD topo probe\n");

    cpuid(0x80000000, &ret);
    largest_extid = ret.a;

    cpuid(0x1, &ret);
    has_x2 = (ret.c >> 21) & 1;


    cpuid(0x80000001, &ret);
    tp->has_topoext = (ret.c >> 22) & 1;

    if (largest_extid >= 0x80000008 && !has_x2) 
        amd_get_topo_legacy(tp);

}


static void 
intel_get_topo_secondary (struct nk_topo_params * tp)
{
	if (cpuid_leaf_max() >= 0xb && has_leafb()) {
		intel_probe_with_leafb(tp);
	}  else {
		intel_probe_with_leaves14(tp);
	}
}


static void
intel_get_topo_params (struct nk_topo_params * tp)
{
    if (has_htt()) {
        intel_get_topo_secondary(tp);
    } else {
        NUMA_DEBUG("No HTT support, using default values\n");
        /* no HTT support, 1 core per package */
        tp->core_only_select_mask = 0;
        tp->smt_mask_width = 0;
        tp->smt_select_mask = 0;
        tp->pkg_select_mask = 0xffffffff;
        tp->pkg_select_mask_width = 0;
    }

    NUMA_DEBUG("Finished Intel topo probe...\n");
    NUMA_DEBUG("\tsmt_mask_width=%d\n", tp->smt_mask_width);
    NUMA_DEBUG("\tsmt_select_mask=0x%x\n", tp->smt_select_mask);
    NUMA_DEBUG("\tcore_plus_mask_width=%d\n", tp->core_plus_mask_width);
    NUMA_DEBUG("\tcore_only_select_mask=0x%x\n", tp->core_only_select_mask);
    NUMA_DEBUG("\tcore_plus_select_mask=0x%x\n", tp->core_plus_select_mask);
    NUMA_DEBUG("\tpkg_select_mask_width=%d\n", tp->pkg_select_mask_width);
    NUMA_DEBUG("\tpkg_select_mask=0x%x\n", tp->pkg_select_mask);

}

static void
amd_get_topo_params (struct nk_topo_params * tp)
{
    amd_get_topo_secondary(tp);
}


static void
get_topo_params (struct nk_topo_params * tp)
{
    if (nk_is_intel()) {
		intel_get_topo_params(tp);
	} else {
		amd_get_topo_params(tp);
    }
}


static void
assign_core_coords_intel (struct cpu * me, struct nk_cpu_coords * coord, struct nk_topo_params *tp)
{
	uint32_t my_apic_id;

	/* 
	 * use the x2APIC ID if it's supported 
	 * and this is the right environment
	 */
	if (cpuid_leaf_max() >= 0xb && has_leafb()) {
		cpuid_ret_t ret;
		cpuid_sub(0xb, 0, &ret);
		NUMA_DEBUG("Using x2APIC for ID\n");
		my_apic_id = ret.d;
	} else {
        cpuid_ret_t ret;
        cpuid_sub(0x1, 0, &ret);
        my_apic_id = ret.b >> 24;
		my_apic_id = apic_get_id(per_cpu_get(apic));
	}

    coord->smt_id  = my_apic_id & tp->smt_select_mask;
    coord->core_id = (my_apic_id & tp->core_only_select_mask) >> tp->smt_mask_width;
    coord->pkg_id  = (my_apic_id & tp->pkg_select_mask) >> tp->core_plus_mask_width;
}


static void
assign_core_coords_amd (struct cpu * me, struct nk_cpu_coords * coord, struct nk_topo_params *tp)
{
    uint32_t my_apic_id = apic_get_id(per_cpu_get(apic));

    if (tp->has_topoext) {
        cpuid_ret_t ret;
        cpuid(0x8000001e, &ret);

        // compute unit is a physical processor (or a CMT core)
        // A "Compute Unit Core" is our equivalent of an SMT thread.
        int cores_per_cu = ((ret.b >> 8) & 0x3) + 1;
        int cu_id = ret.b & 0xff;

        coord->smt_id  = my_apic_id % cores_per_cu;
        coord->core_id = cu_id;
        // "Node" is a package (die). On MCM systems we can
        // have more than one die (package) in a processor (socket).
        // TODO: we are not guaranteed to get linear numbering here
        // for package IDs
        coord->pkg_id  = (uint32_t)(ret.c & 0xff);
    } else {
        uint32_t logprocid = (tp->max_ncores !=0) ? my_apic_id % tp->max_ncores : 0;
        coord->smt_id  = (tp->max_nthreads != 0) ? (my_apic_id % tp->max_nthreads) : 0;
        coord->core_id = (tp->max_nthreads != 0) ? (logprocid / tp->max_nthreads) : 0;
        coord->pkg_id  = (tp->max_ncores != 0) ? (my_apic_id / tp->max_ncores) : 0;
    }
}

static void 
assign_core_coords (struct cpu * me, struct nk_cpu_coords * coord, struct nk_topo_params *tp)
{

	/* 
	 * use the x2APIC ID if it's supported 
	 * and this is the right environment
	 */
	if (nk_is_intel())
		assign_core_coords_intel(me, coord, tp);
	else
		assign_core_coords_amd(me, coord, tp);

    NUMA_DEBUG("Core OS ID: %u:\n", me->id);
    NUMA_DEBUG("\tLogical Core ID:  %u\n", coord->smt_id);
    NUMA_DEBUG("\tPhysical Core ID: %u\n", coord->core_id);
    NUMA_DEBUG("\tPhysical Package ID: %u\n", coord->pkg_id);
}

/* 
 *
 * to be called by all cores
 *
 */
int
nk_cpu_topo_discover (struct cpu * me)
{
    struct nk_cpu_coords * coord = NULL;
    struct nk_topo_params * tp = NULL;

    coord = (struct nk_cpu_coords*)malloc(sizeof(struct nk_cpu_coords));
    if (!coord) {
        NUMA_ERROR("Could not allocate coord struct for CPU %u\n", my_cpu_id());
        return -1;
    }
    memset(coord, 0, sizeof(struct nk_cpu_coords));

    tp = (struct nk_topo_params*)malloc(sizeof(struct nk_topo_params));
    if (!tp) {
        NUMA_ERROR("Could not allocate param struct for CPU %u\n", my_cpu_id());
        goto out_err;
    }
    memset(tp, 0, sizeof(struct nk_topo_params));

    get_topo_params(tp);

    /* now we can determine the CPU coordinates */
    assign_core_coords(me, coord, tp);

    me->tp    = tp;
    me->coord = coord;

    return 0;

out_err:
    free(coord);
    return -1;
}

static void 
acpi_table_print_srat_entry (struct acpi_subtable_header * header)
{

    ACPI_FUNCTION_NAME("acpi_table_print_srat_entry");

    if (!header)
        return;

    switch (header->type) {

    case ACPI_SRAT_TYPE_CPU_AFFINITY:
        {
            struct acpi_srat_cpu_affinity *p =
                container_of(header, struct acpi_srat_cpu_affinity, header);
            u32 proximity_domain = p->proximity_domain_lo;

            if (srat_rev >= 2) {
                proximity_domain |= p->proximity_domain_hi[0] << 8;
                proximity_domain |= p->proximity_domain_hi[1] << 16;
                proximity_domain |= p->proximity_domain_hi[2] << 24;
            }
            NUMA_DEBUG("SRAT Processor (id[0x%02x] eid[0x%02x]) in proximity domain %d %s\n",
                      p->apic_id, p->local_sapic_eid,
                      proximity_domain,
                      p->flags & ACPI_SRAT_CPU_ENABLED
                      ? "enabled" : "disabled");
        }
        break;

    case ACPI_SRAT_TYPE_MEMORY_AFFINITY:
        {
            struct acpi_srat_mem_affinity *p =
                container_of(header, struct acpi_srat_mem_affinity, header);
            u32 proximity_domain = p->proximity_domain;

            if (srat_rev < 2)
                proximity_domain &= 0xff;
            NUMA_DEBUG("SRAT Memory (0x%016llx length 0x%016llx type 0x%x) in proximity domain %d %s%s\n",
                      p->base_address, p->length,
                      p->memory_type, proximity_domain,
                      p->flags & ACPI_SRAT_MEM_ENABLED
                      ? "enabled" : "disabled",
                      p->flags & ACPI_SRAT_MEM_HOT_PLUGGABLE
                      ? " hot-pluggable" : "");
        }
        break;

    case ACPI_SRAT_TYPE_X2APIC_CPU_AFFINITY:
        {
            struct acpi_srat_x2apic_cpu_affinity *p =
                (struct acpi_srat_x2apic_cpu_affinity *)header;
            NUMA_DEBUG("SRAT Processor (x2apicid[0x%08x]) in"
                      " proximity domain %d %s\n",
                      p->apic_id,
                      p->proximity_domain,
                      (p->flags & ACPI_SRAT_CPU_ENABLED) ?
                      "enabled" : "disabled");
        }
        break;
    default:
        NUMA_DEBUG("Found unsupported SRAT entry (type = 0x%x)\n",
               header->type);
        break;
    }
}

static int 
acpi_parse_x2apic_affinity(struct acpi_subtable_header *header,
               const unsigned long end)
{
    struct acpi_srat_x2apic_cpu_affinity *processor_affinity;

    processor_affinity = (struct acpi_srat_x2apic_cpu_affinity *)header;

    if (!processor_affinity) {
        return -EINVAL;
    }

    struct sys_info * sys = &(nk_get_nautilus_info()->sys);
    unsigned i;
    uint32_t domain_id;

    if (!(processor_affinity->flags & ACPI_SRAT_CPU_ENABLED)) { 
	NUMA_DEBUG("Processor affinity for disabled x2apic processor...\n");
	return 0;
    }

    acpi_table_print_srat_entry(header);

    for (i = 0; i < sys->num_cpus; i++) {
        if (sys->cpus[i]->lapic_id == processor_affinity->apic_id) {
            domain_id = processor_affinity->proximity_domain;
            sys->cpus[i]->domain = sys->locality_info.domains[domain_id];
            break;
        }
    }

    if (i==sys->num_cpus) {
	NUMA_ERROR("Affinity for x2apic processor %x not found\n",processor_affinity->apic_id);
    }

    return 0;
}


static int 
acpi_table_parse_srat(enum acpi_srat_entry_id id,
              acpi_madt_entry_handler handler, unsigned int max_entries)
{
    return acpi_table_parse_entries(ACPI_SIG_SRAT,
                    sizeof(struct acpi_table_srat), id,
                    handler, max_entries);
}


static inline int
domain_exists (struct sys_info * sys, unsigned id)
{
    return sys->locality_info.domains[id] != NULL;
}

static inline struct numa_domain *
get_domain (struct sys_info * sys, unsigned id)
{
    return sys->locality_info.domains[id];
}




static int 
acpi_parse_processor_affinity(struct acpi_subtable_header * header,
                              const unsigned long end)
{
    struct acpi_srat_cpu_affinity *processor_affinity
        = container_of(header, struct acpi_srat_cpu_affinity, header);

    struct sys_info * sys = &(nk_get_nautilus_info()->sys);
    uint32_t i, domain_id;

    if (!processor_affinity)
        return -EINVAL;

    if (!(processor_affinity->flags & ACPI_SRAT_CPU_ENABLED)) { 
		NUMA_DEBUG("Processor affinity for disabled processor...\n");
		return 0;
    }

    domain_id = processor_affinity->proximity_domain_lo | 
                (((*(uint32_t*)(&(processor_affinity->proximity_domain_hi))) & 0xffffff) << 8);

    acpi_table_print_srat_entry(header);

    /* add this domain if we haven't seen it yet */
    if (!domain_exists(sys, domain_id)) {
        if (nk_numa_domain_create(sys, domain_id) == NULL) {
            NUMA_ERROR("Could not add NUMA domain for processor affinity\n");
            return -1;
        }
    }

    for (i = 0; i < sys->num_cpus; i++) {
        if (sys->cpus[i]->lapic_id == processor_affinity->apic_id) {
            sys->cpus[i]->domain = get_domain(sys, domain_id);
            break;
        }
    }

    if (i==sys->num_cpus) {
		NUMA_ERROR("Affinity for processor %d not found\n",processor_affinity->apic_id);
    }

    return 0;
}


/*
 * This is where we construct all of our NUMA domain 
 * information 
 */
static int 
acpi_parse_memory_affinity(struct acpi_subtable_header * header,
               const unsigned long end)
{
    struct acpi_srat_mem_affinity *memory_affinity
        = container_of(header, struct acpi_srat_mem_affinity, header);

    struct sys_info * sys       = &(nk_get_nautilus_info()->sys);
    struct mem_region * mem     = NULL;
    struct numa_domain * domain = NULL;
    struct mem_region * ent     = NULL;

    if (!memory_affinity)
        return -EINVAL;

    if (!(memory_affinity->flags & ACPI_SRAT_MEM_ENABLED)) {
		NUMA_DEBUG("Disabled memory region affinity...\n");
		return 0;
    }

    if (memory_affinity->length == 0 ) { 
		NUMA_DEBUG("Whacky length zero memory region...\n");
    }

    acpi_table_print_srat_entry(header);

    mem = mm_boot_alloc(sizeof(struct mem_region));
    if (!mem) {
        ERROR_PRINT("Could not allocate mem region\n");
        return -1;
    }
    memset(mem, 0, sizeof(struct mem_region));

    mem->domain_id     = memory_affinity->proximity_domain & ((srat_rev < 2) ? 0xff : 0xffffffff);
    mem->base_addr     = memory_affinity->base_address;
    mem->len           = memory_affinity->length;
    mem->enabled       = memory_affinity->flags & ACPI_SRAT_MEM_ENABLED;
    mem->hot_pluggable = memory_affinity->flags & ACPI_SRAT_MEM_HOT_PLUGGABLE;
    mem->nonvolatile   = memory_affinity->flags & ACPI_SRAT_MEM_NON_VOLATILE;

    ASSERT(mem->domain_id < MAX_NUMA_DOMAINS);

    NUMA_DEBUG("Memory region: domain 0x%lx, base %p, len 0x%lx, enabled=%d hot_plug=%d nv=%d\n", mem->domain_id, mem->base_addr, mem->len, mem->enabled, mem->hot_pluggable, mem->nonvolatile);

    /* we've detected a new NUMA domain */
    if (!domain_exists(sys, mem->domain_id)) {

        NUMA_DEBUG("Region is in new domain (%u)\n", mem->domain_id);

        domain = nk_numa_domain_create(sys, mem->domain_id);

        if (!domain) {
            ERROR_PRINT("Could not create NUMA domain (id=%u)\n", mem->domain_id);
            return -1;
        }
        
    } else {
        NUMA_DEBUG("Region is in existing domain\n");
        domain = get_domain(sys, mem->domain_id);
    }

    domain->num_regions++;
    domain->addr_space_size += mem->len;

    NUMA_DEBUG("Domain %u now has 0x%lx regions and size 0x%lx\n", domain->id, domain->num_regions, domain->addr_space_size);

    if (list_empty(&domain->regions)) {
        list_add(&mem->entry, &domain->regions);
        return 0;
    } else {
        list_for_each_entry(ent, &domain->regions, entry) {
            if (mem->base_addr < ent->base_addr) {
                NUMA_DEBUG("Added region prior to region with base address 0x%lx\n",ent->base_addr);
                list_add_tail(&mem->entry, &ent->entry);
                return 0;
            }
        }
        list_add_tail(&mem->entry, &domain->regions);
    }

    return 0;
}


static int
acpi_parse_srat (struct acpi_table_header * hdr, void * arg)
{
    NUMA_DEBUG("Parsing SRAT...\n");
    srat_rev = hdr->revision;

    return 0;
}

/* NOTE: this is an optional table, and chances are we may not even see it */
static int 
acpi_parse_slit(struct acpi_table_header *table, void * arg)
{
    struct nk_locality_info * numa = (struct nk_locality_info*)arg;
    struct acpi_table_slit * slit = (struct acpi_table_slit*)table;
#ifdef NAUT_CONFIG_DEBUG_NUMA
    unsigned i, j;

    NUMA_DEBUG("Parsing SLIT...\n");
    NUMA_DEBUG("Locality Count: %llu\n", slit->locality_count);

    NUMA_DEBUG("  Entries:\n");
    NUMA_DEBUG("   ");
    for (i = 0; i < slit->locality_count; i++) {
        printk("%02u ", i);
    }
    printk("\n");

    for (i = 0; i < slit->locality_count; i++) {
        NUMA_DEBUG("%02u ", i);
        for (j = 0; j < slit->locality_count; j++) {
            printk("%02u ", *(uint8_t*)(slit->entry + i*slit->locality_count + j));
        }
        printk("\n");
    }

    NUMA_DEBUG("DONE.\n");

#endif

    numa->numa_matrix = mm_boot_alloc(slit->locality_count * slit->locality_count);
    if (!numa->numa_matrix) {
        ERROR_PRINT("Could not allocate NUMA matrix\n");
        return -1;
    }

    memcpy((void*)numa->numa_matrix, 
           (void*)slit->entry, 
           slit->locality_count * slit->locality_count);

    return 0;
}


/* 
 * Assumes that nk_acpi_init() has 
 * already been called 
 *
 */
int 
arch_numa_init (struct sys_info * sys)
{
    NUMA_PRINT("Parsing ACPI NUMA information...\n");

    /* SLIT: System Locality Information Table */
    if (acpi_table_parse(ACPI_SIG_SLIT, acpi_parse_slit, &(sys->locality_info))) { 
        NUMA_DEBUG("Unable to parse SLIT\n");
    }

    /* SRAT: Static Resource Affinity Table */
    if (!acpi_table_parse(ACPI_SIG_SRAT, acpi_parse_srat, &(sys->locality_info))) {

        NUMA_DEBUG("Parsing SRAT_MEMORY_AFFINITY table...\n");

        if (acpi_table_parse_srat(ACPI_SRAT_MEMORY_AFFINITY,
                    acpi_parse_memory_affinity, NAUT_CONFIG_MAX_CPUS * 2) < 0) {
            NUMA_ERROR("Unable to parse memory affinity\n");
        }

        NUMA_DEBUG("DONE.\n");

        NUMA_DEBUG("Parsing SRAT_TYPE_X2APIC_CPU_AFFINITY table...\n");

        if (acpi_table_parse_srat(ACPI_SRAT_X2APIC_CPU_AFFINITY,
                    acpi_parse_x2apic_affinity, NAUT_CONFIG_MAX_CPUS) < 0)	    {
            NUMA_ERROR("Unable to parse x2apic table\n");
        }

        NUMA_DEBUG("DONE.\n");

        NUMA_DEBUG("Parsing SRAT_PROCESSOR_AFFINITY table...\n");

        if (acpi_table_parse_srat(ACPI_SRAT_PROCESSOR_AFFINITY,
                    acpi_parse_processor_affinity,
                    NAUT_CONFIG_MAX_CPUS) < 0) { 
            NUMA_ERROR("Unable to parse processor affinity\n");
        }

        NUMA_DEBUG("DONE.\n");

    }

    NUMA_PRINT("DONE.\n");

    return 0;
}
