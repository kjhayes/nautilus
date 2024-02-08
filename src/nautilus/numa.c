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
 * Copyright (c) 2020, Kyle C. Hale <khale@cs.iit.edu>
 * Copyright (c) 2020, The V3VEE Project  <http://www.v3vee.org> 
 *                     The Hobbes Project <http://xstack.sandia.gov/hobbes>
 * All rights reserved.
 *
 * Author: Kyle C. Hale <khale@cs.iit.edu>
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "LICENSE.txt".
 */
#include <nautilus/nautilus.h>
#include <nautilus/numa.h>
#include <nautilus/errno.h>
#include <nautilus/list.h>
#include <nautilus/percpu.h>
#include <nautilus/math.h>
#include <nautilus/paging.h>
#include <nautilus/mm.h>
#include <nautilus/atomic.h>
#include <nautilus/naut_string.h>
#include <nautilus/multiboot2.h>
#include <dev/apic.h>

#ifdef NAUT_CONFIG_ARCH_X86
#include <arch/x64/msr.h>
#include <arch/x64/cpu.h>
#endif

#ifndef NAUT_CONFIG_DEBUG_NUMA
#undef DEBUG_PRINT
#define DEBUG_PRINT(fmt, args...)
#endif

#define NUMA_PRINT(fmt, args...) printk("NUMA: " fmt, ##args)
#define NUMA_DEBUG(fmt, args...) DEBUG_PRINT("NUMA: " fmt, ##args)
#define NUMA_ERROR(fmt, args...) ERROR_PRINT("NUMA: " fmt, ##args)
#define NUMA_WARN(fmt, args...)  WARN_PRINT("NUMA: " fmt, ##args)

static void 
dump_mem_regions (struct numa_domain * d)
{
    struct mem_region * reg = NULL;
    unsigned i = 0;

    list_for_each_entry(reg, &(d->regions), entry) {
        printk("\tMemory Region %u:\n", i);
        printk("\t\tRange:         %p - %p\n", (void*)reg->base_addr, (void*)(reg->base_addr + reg->len));
        printk("\t\tSize:          %lu.%lu MB\n", reg->len / 1000000, reg->len % 1000000);
        printk("\t\tEnabled:       %s\n", reg->enabled ? "Yes" : "No");
        printk("\t\tHot Pluggable: %s\n", reg->hot_pluggable ? "Yes" : "No");
        printk("\t\tNon-volatile:  %s\n", reg->nonvolatile ? "Yes" : "No");
        i++;
    }
}


static void
dump_adj_list (struct numa_domain * d)
{
    struct domain_adj_entry * ent = NULL;

    if (list_empty(&d->adj_list)) {
        printk("[no entries in adjacency list]\n");
        return;
    }
    printk("[ ");
    list_for_each_entry(ent, &(d->adj_list), list_ent) {
        printk("%u ", ent->domain->id);
    }
    printk("]\n");
}


static void
dump_domain_cpus (struct numa_domain * d)
{
    struct sys_info * sys = &(nk_get_nautilus_info()->sys);
    int i;

    printk("[ ");

    for (i = 0; i < sys->num_cpus; i++) {
        if (sys->cpus[i]->domain == d) {
            printk("%u ", i);
        }
    }

    printk(" ]\n");
}


static void
dump_numa_domains (struct nk_locality_info * numa)
{
    unsigned i;

    printk("%u NUMA Domains:\n", numa->num_domains);
    printk("----------------\n");

    for (i = 0; i < numa->num_domains; i++) {

        struct numa_domain * domain = numa->domains[i];

        printk("Domain %u (Total Size = %lu.%lu MB)\n",
                domain->id,
                domain->addr_space_size / 1000000,
                domain->addr_space_size % 1000000);

        printk("Adjacency List (ordered by distance): ");
        dump_adj_list(domain);

        printk("Associated CPUs: ");
        dump_domain_cpus(domain);

        printk("%u constituent region%s:\n", domain->num_regions, domain->num_regions > 1 ? "s" : "");

        dump_mem_regions(domain);
        printk("\n");
    }
}


static void 
dump_numa_matrix (struct nk_locality_info * numa)
{
    unsigned i,j;

    if (!numa || !numa->numa_matrix) {
        return;
    }

    printk("NUMA Distance Matrix:\n");
    printk("---------------------\n");
    printk("   ");

    for (i = 0; i < numa->num_domains; i++) {
        printk("%02u ", i);
    }
    printk("\n");

    for (i = 0; i < numa->num_domains; i++) {
        printk("%02u ", i);
        for (j = 0; j < numa->num_domains; j++) {
            printk("%02u ", *(numa->numa_matrix + i*numa->num_domains + j));
        }
        printk("\n");
    }

}


static void 
__associate_domains_slit (struct nk_locality_info * loc)
{
    unsigned i, j;

    NUMA_DEBUG("Associate Domains Using SLIT\n");
    
    for (i = 0; i < loc->num_domains; i++) {
        for (j = 0; j < loc->num_domains; j++) {
            if (j == i) continue;

            struct domain_adj_entry * new_dom_ent = mm_boot_alloc(sizeof(struct domain_adj_entry));
            new_dom_ent->domain = loc->domains[j];
            if (list_empty(&(loc->domains[i]->adj_list))) {
                list_add(&(new_dom_ent->list_ent), &(loc->domains[i]->adj_list));
            } else {
                struct domain_adj_entry * ent = NULL;
                list_for_each_entry(ent, &(loc->domains[i]->adj_list), list_ent) {
                    uint8_t dist_to_j = *(loc->numa_matrix + i*loc->num_domains + j);
                    uint8_t dist_to_other = *(loc->numa_matrix + i*loc->num_domains + ent->domain->id);

                    if (dist_to_j < dist_to_other) {
                        list_add_tail(&(new_dom_ent->list_ent), &(ent->list_ent));
                        continue;
                    }
                }
                list_add_tail(&(new_dom_ent->list_ent), &(loc->domains[i]->adj_list));
            }

        }
    }
}


static void
__associate_domains_adhoc (struct nk_locality_info * loc)
{
    unsigned i, j;
    
    NUMA_WARN("Associate Domains using ADHOC METHOD\n");

    for (i = 0; i < loc->num_domains; i++) {
        for (j = 0; j < loc->num_domains; j++) {
            if (j == i) continue;

            struct domain_adj_entry * new_dom_ent = mm_boot_alloc(sizeof(struct domain_adj_entry));
	    if (!new_dom_ent) {
		NUMA_ERROR("Unable to allocate domain entry - skipping association\n");
		continue;
	    }
	    INIT_LIST_HEAD(&new_dom_ent->list_ent);
            new_dom_ent->domain = loc->domains[j];
            NUMA_DEBUG("Adding domain %u to adjacency list of domain %u\n", j, i);
            list_add(&(new_dom_ent->list_ent), &(loc->domains[i]->adj_list));
        }
    }
}


static void
__coalesce_regions (struct numa_domain * d)
{
    struct mem_region * mem = NULL;
    unsigned i;

    NUMA_DEBUG("Coalesce Regions for domain id=%lu\n",d->id);

restart_scan:
    NUMA_DEBUG("Restart Scan\n");
    i = 0;
    list_for_each_entry(mem, &d->regions, entry) {

        if (mem->entry.next != &d->regions) {

            NUMA_DEBUG("Examining region:  base=%p, len=0x%lx, domain=0x%lx enabled=%d hot_plug=%d nv=%d\n",mem->base_addr, mem->len, mem->domain_id, mem->enabled, mem->hot_pluggable, mem->nonvolatile);
            struct mem_region * next_mem = list_entry(mem->entry.next, struct mem_region, entry);
            ASSERT(next_mem);

            NUMA_DEBUG("Next region is: base=%p, len=0x%lx, domain=0x%lx, enabled=%d, hot_plug=%d nv=%d\n",next_mem->base_addr, next_mem->len, next_mem->domain_id, next_mem->enabled, next_mem->hot_pluggable, next_mem->nonvolatile);

            if (next_mem->base_addr == mem->base_addr + mem->len) {

                NUMA_DEBUG("Coalescing adjacent regions %u and %u in domain %u\n", i, i+1, d->id);
                NUMA_DEBUG("First  region is base=%p, len=0x%lx, domain=0x%lx enabled=%d\n",mem->base_addr, mem->len, mem->domain_id, mem->enabled);
                NUMA_DEBUG("Second region is base=%p, len=0x%lx, domain=0x%lx enabled=%d\n", next_mem->base_addr, next_mem->len, next_mem->domain_id, next_mem->enabled);
                struct mem_region * new_reg = mm_boot_alloc(sizeof(struct mem_region));
                if (!new_reg) {
                    panic("Could not create new coalesced memory region\n");
                }

                memset(new_reg, 0, sizeof(struct mem_region));

                /* NOTE: we ignore the flags, e.g. nonvolatile, hot removable etc */
                new_reg->base_addr = mem->base_addr;
                new_reg->len       = mem->len + next_mem->len;
                new_reg->domain_id = mem->domain_id;
                new_reg->enabled   = mem->enabled;

                NUMA_DEBUG("Combined Region is base=%p, len=0x%lx, domain=0x%lx enabled=%d\n", new_reg->base_addr, new_reg->len, new_reg->domain_id, new_reg->enabled);

                /* stitch the coalesced region into the domain's region list */
                list_add_tail(&new_reg->entry, &mem->entry);

                /* get rid of the old ones */
                list_del(&mem->entry);
                list_del(&next_mem->entry);

                --d->num_regions;
                goto restart_scan;

            }

        } 
        i++;
    }
}


/*
 * sometimes we end up with non-overlapping, adjacent regions
 * that, for our purposes, should be treated as one range of memory.
 * We coalesce them here to avoid overhead in the page allocator
 * later on.
 */
static void
coalesce_regions (struct nk_locality_info * loc)
{
    unsigned i;

    NUMA_DEBUG("Coalescing adjacent memory regions\n");
    for (i = 0; i < loc->num_domains; i++) {
        NUMA_DEBUG("...coalescing for domain %d\n", i);
        __coalesce_regions(loc->domains[i]);
    }
}


static void
associate_domains (struct nk_locality_info * loc)
{
    NUMA_DEBUG("Associating NUMA domains to each other using adjacency info\n");
    if (loc->numa_matrix) {
        __associate_domains_slit(loc);
    } else {
        __associate_domains_adhoc(loc);
    }
}


struct mem_region *
nk_get_base_region_by_num (unsigned num)
{
    struct nk_locality_info * loc = &(nautilus_info.sys.locality_info);

    ASSERT(num < loc->num_domains);

    struct list_head * first = loc->domains[num]->regions.next;
    return list_entry(first, struct mem_region, entry);
}


struct mem_region * 
nk_get_base_region_by_cpu (cpu_id_t cpu)
{
    struct numa_domain * domain = nk_get_nautilus_info()->sys.cpus[cpu]->domain;
    struct list_head * first = domain->regions.next;
    return list_entry(first, struct mem_region, entry);
}


unsigned
nk_my_numa_node (void)
{
    return per_cpu_get(domain)->id;
}


void
nk_dump_numa_info (void)
{
    struct nk_locality_info * numa = &(nk_get_nautilus_info()->sys.locality_info);

    printk("===================\n");
    printk("     NUMA INFO:\n");
    printk("===================\n");
    
    dump_numa_domains(numa);

    dump_numa_matrix(numa);

    printk("\n");
    printk("======================\n");
    printk("     END NUMA INFO:\n");
    printk("======================\n");
}

unsigned 
nk_get_num_domains (void)
{
    struct nk_locality_info * l = &(nk_get_nautilus_info()->sys.locality_info);

    return l->num_domains;
}


struct numa_domain *
nk_numa_domain_create (struct sys_info * sys, unsigned id)
{
    struct numa_domain * d = NULL;

    d = (struct numa_domain *)mm_boot_alloc(sizeof(struct numa_domain));
    if (!d) {
        ERROR_PRINT("Could not allocate NUMA domain\n");
        return NULL;
    }
    memset(d, 0, sizeof(struct numa_domain));

    d->id = id;

    INIT_LIST_HEAD(&(d->regions));
    INIT_LIST_HEAD(&(d->adj_list));

    if (id != (sys->locality_info.num_domains + 1)) { 
        NUMA_DEBUG("Memory regions are not in expected domain order, but that should be OK\n");
    }

    sys->locality_info.domains[id] = d;
    sys->locality_info.num_domains++;

    return d;
}


/*
 *
 * only called by BSP once
 *
 */
int
nk_numa_init (void)
{
    struct sys_info * sys = &(nk_get_nautilus_info()->sys);

    NUMA_DEBUG("NUMA ARCH INIT\n");

    if (arch_numa_init(sys) != 0) {
        NUMA_ERROR("Error initializing arch-specific NUMA\n");
        return -1;
    }

    /* if we didn't find any of the relevant ACPI tables, we need
     * to coalesce the regions given to us at boot-time into a single
     * NUMA domain */
    if (nk_get_num_domains() == 0) {

        NUMA_DEBUG("No domains in ACPI locality info - making it up\n");

        int i;
        struct numa_domain * domain = nk_numa_domain_create(sys, 0);

        if (!domain) {
            NUMA_ERROR("Could not create main NUMA domain\n");
            return -1;
        }

        for (i = 0; i < mm_boot_num_regions(); i++) {
            struct mem_map_entry * m = mm_boot_get_region(i);
            struct mem_region * region = NULL;
            if (!m) {
                NUMA_ERROR("Couldn't get memory map region %u\n", i);
                return -1;
            }

            if (m->type != MULTIBOOT_MEMORY_AVAILABLE) {
                continue;
            }

            region = mm_boot_alloc(sizeof(struct mem_region));
            if (!region) {
                NUMA_ERROR("Couldn't allocate memory region\n");
                return -1;
            }
            memset(region, 0, sizeof(struct mem_region));
            
            region->domain_id     = 0;
            region->base_addr     = m->addr;
            region->len           = m->len;
            region->enabled       = 1;
            region->hot_pluggable = 0;
            region->nonvolatile   = 0;
        
            domain->num_regions++;
            domain->addr_space_size += region->len;

            /* add the mem region in order */
            if (list_empty(&domain->regions)) {
                list_add(&region->entry, &domain->regions);
            } else {
                struct mem_region * ent = NULL;
                list_for_each_entry(ent, &domain->regions, entry) {
                    if (region->base_addr < ent->base_addr) {
                        list_add_tail(&region->entry, &ent->entry);
                        break;
                    }
                }
                if (ent != (struct mem_region *)&domain->regions) {
                    list_add_tail(&region->entry, &domain->regions);
                }
            }

        }

        /* this domain is now every CPU's local domain */
        for (i = 0; i < sys->num_cpus; i++) {
            sys->cpus[i]->domain = domain;
        }
    }


    // now deal with weird situations where they start counting domains at 1
    // should ideally find all "missing" domains and create fakes for them
    if (!sys->locality_info.domains[0]) {
	NUMA_WARN("Faking domain 0 because of crack monkeys\n");
	struct numa_domain *n = nk_numa_domain_create(sys,0);
	if (!n) {
	    NUMA_ERROR("Cannot allocate fake domain 0\n");
	    return -1;
	}
	// domain 0 should be zero size, 0 regions, no adjacency
    }

    
    /* we now construct a list of adjacent domains for each domain, 
     * ordered by distances obtained in the SLIT. If we didn't see a SLIT,
     * we add domains arbitrarily */
    associate_domains(&sys->locality_info);

    /* combine non-overlapping adjacent regions */
    coalesce_regions(&sys->locality_info);

#ifdef NAUT_CONFIG_DEBUG_NUMA
    nk_dump_numa_info();
#endif

    return 0;
}

