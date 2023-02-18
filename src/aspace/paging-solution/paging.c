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
 * Copyright (c) 2019, Hongyi Chen
 * Copyright (c) 2019, Peter Dinda
 * Copyright (c) 2019, The V3VEE Project  <http://www.v3vee.org> 
 *                     The Hobbes Project <http://xstack.sandia.gov/hobbes>
 * All rights reserved.
 *
 * Authors: Hongyi Chen
 *          Peter Dinda <pdinda@northwestern.edu>
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "LICENSE.txt".
 */


//
// This is a template for the CS343 paging lab at
// Northwestern University
//
// Please also look at the paging_helpers files!
//
//
//
//

#include <nautilus/nautilus.h>
#include <nautilus/spinlock.h>
#include <nautilus/paging.h>
#include <nautilus/thread.h>
#include <nautilus/shell.h>
#include <nautilus/cpu.h>

#include <nautilus/aspace.h>

#include "paging_helpers.h"
#include "region_list.h"

// Signal handling stuff
#ifdef NAUT_CONFIG_ENABLE_USERSPACE
#include <nautilus/user.h>
#endif


//
// Add debugging and other optional output to this subsytem
//
#ifndef NAUT_CONFIG_DEBUG_ASPACE_PAGING
#undef DEBUG_PRINT
#define DEBUG_PRINT(fmt, args...) 
#endif

#define ERROR(fmt, args...) ERROR_PRINT("aspace-paging: " fmt, ##args)
#define DEBUG(fmt, args...) DEBUG_PRINT("aspace-paging: " fmt, ##args)
#define INFO(fmt, args...)   INFO_PRINT("aspace-paging: " fmt, ##args)


// Some macros to hide the details of doing locking for
// a paging address space
#define ASPACE_LOCK_CONF uint8_t _aspace_lock_flags
#define ASPACE_LOCK(a) _aspace_lock_flags = spin_lock_irq_save(&(a)->lock)
#define ASPACE_TRY_LOCK(a) spin_try_lock_irq_save(&(a)->lock,&_aspace_lock_flags)
#define ASPACE_UNLOCK(a) spin_unlock_irq_restore(&(a)->lock, _aspace_lock_flags);
#define ASPACE_UNIRQ(a) irq_enable_restore(_aspace_lock_flags);


// graceful printouts of names
#define ASPACE_NAME(a) ((a)?(a)->aspace->name : "default")
#define THREAD_NAME(t) ((!(t)) ? "(none)" : (t)->is_idle ? "(idle)" : (t)->name[0] ? (t)->name : "(noname)")
#define REGION_FORMAT "(VA=0x%p to PA=0x%p, len=%lx, prot=%lx)"
#define REGION(r) (r)->va_start, (r)->pa_start, (r)->len_bytes, (r)->protect.flags

/**
 *  should be defined in aspace.h, but moved here for the simplicity of solution
 * */
#define NK_ASPACE_GET_READ(flags) ((flags & NK_ASPACE_READ) >> 0)
#define NK_ASPACE_GET_WRITE(flags) ((flags & NK_ASPACE_WRITE) >> 1)
#define NK_ASPACE_GET_EXEC(flags) ((flags & NK_ASPACE_EXEC) >> 2)
#define NK_ASPACE_GET_PIN(flags) ((flags & NK_ASPACE_PIN) >> 3)
#define NK_ASPACE_GET_KERN(flags) ((flags & NK_ASPACE_KERN) >> 4)
#define NK_ASPACE_GET_SWAP(flags) ((flags & NK_ASPACE_SWAP) >> 5)
#define NK_ASPACE_GET_EAGER(flags) ((flags & NK_ASPACE_EAGER) >> 6)

/**
 *  subject to design
 * */
#define THRESH PAGE_SIZE_2MB


// You probably want some sort of data structure that will let you
// keep track of the set of regions you are asked to add/remove/change
typedef struct region_node {
    nk_aspace_region_t region;
    // linked list ?  tree?  ?? 
} region_node_t;

// You will want some data structure to represent the state
// of a paging address space
typedef struct nk_aspace_paging {
    // pointer to the abstract aspace that the
    // rest of the kernel uses when dealing with this
    // address space
    nk_aspace_t *aspace;
    
    // perhaps you will want to do concurrency control?
    spinlock_t   lock;

    // Here you probably will want your region set data structure 
    // What should it be...
    
    // Your characteristics
    nk_aspace_characteristics_t chars;
    
    mm_llist_t * llist_tracker;

    // The cr3 register contents that reflect
    // the root of your page table hierarchy
    ph_cr3e_t     cr3;

    // The cr4 register contents used by the HW to interpret
    // your page table hierarchy.   We only care about a few bits
#define CR4_MASK 0xb0ULL // bits 4,5,7
    uint64_t      cr4;

} nk_aspace_paging_t;


static int remove_region(void *state, nk_aspace_region_t *region);

// The function the aspace abstraction will call when it
// wants to destroy your address space
static  int destroy(void *state)
{
    // the pointer it hands you is for the state you supplied
    // when you registered the address space
    nk_aspace_paging_t *p = (nk_aspace_paging_t *)state;

    DEBUG("destroying address space %s\n", ASPACE_NAME(p));

    ASPACE_LOCK_CONF;

    // lets do that with a lock, perhaps? 
    ASPACE_LOCK(p);
    //
    // WRITEME!!    actually do the work
    // 
    nk_aspace_region_t regions_to_unmap[p->llist_tracker->size];
    off_t ind = 0;
    mm_llist_node_t * curr_node = p->llist_tracker->region_head;
    while (curr_node != NULL) {
        regions_to_unmap[ind++] = curr_node->region;
        // printk("VA = %016lx to PA = %016lx, len = %16lx\n", 
        //     curr_node->region.va_start,
        //     curr_node->region.pa_start,
        //     curr_node->region.len_bytes
        // );
        curr_node = curr_node->next_llist_node;
    }


    ASPACE_UNLOCK(p);

    for (int i = 0; i < p->llist_tracker->size; i++) {
        remove_region(state, &regions_to_unmap[i]);
    }

    return 0;
}

// The function the aspace abstraction will call when it
// is adding a thread to your address space
// do you care? 
static int add_thread(void *state)
{
    nk_aspace_paging_t *p = (nk_aspace_paging_t *)state;
    struct nk_thread *t = get_cur_thread();
    
    DEBUG("adding thread %d (%s) to address space %s\n", t->tid,THREAD_NAME(t), ASPACE_NAME(p));
    
    return 0;
}
    
    
// The function the aspace abstraction will call when it
// is removing from your address space
// do you care? 
static int remove_thread(void *state)
{
    nk_aspace_paging_t *p = (nk_aspace_paging_t *)state;
    struct nk_thread *t = get_cur_thread();
    
    DEBUG("removing thread %d (%s) from address space %s\n", t->tid, THREAD_NAME(t), ASPACE_NAME(p));
    
    return 0;
}

ph_pf_access_t access_from_region (nk_aspace_region_t *region) {
    ph_pf_access_t access;
    access.val = 0;
    
    access.write = NK_ASPACE_GET_WRITE(region->protect.flags);
    access.user = !NK_ASPACE_GET_KERN(region->protect.flags);
    access.ifetch = NK_ASPACE_GET_EXEC(region->protect.flags);

    return access;
}

int region_align_check(nk_aspace_paging_t *p, nk_aspace_region_t *region) {
    // sanity check for region validness
    if ((addr_t) region->va_start % p->chars.alignment != 0) {
        ERROR("region VA_start=0x%p expect to have alignment of %lx", region->va_start, p->chars.alignment);
        return -1;
    }

    if ((addr_t) region->pa_start % p->chars.alignment != 0) {
        ERROR("region PA_start=0x%p expect to have alignment of %lx", region->pa_start, p->chars.alignment);
        return -1;
    }

    if ((addr_t) region->len_bytes % p->chars.granularity != 0) {
        ERROR("region len_bytes=0x%lx expect to have granularity of %lx", region->len_bytes, p->chars.granularity);
        return -1;
    }
    return 0;
}

int clear_cache (nk_aspace_paging_t *p, nk_aspace_region_t *region, uint64_t threshold) {
    
    // if we are editing the current address space of this cpu, then we
    // might need to flush the TLB here.   We can do that with a cr3 write
    // like: write_cr3(p->cr3.val);

    // if this aspace is active on a different cpu, we might need to do
    // a TLB shootdown here (out of scope of class)
    // a TLB shootdown is an interrupt to a remote CPU whose handler
    // flushes the TLB

    if (p->aspace == get_cpu()->cur_aspace) {
            // write_cr3(p->cr3.val);
            // DEBUG("flush TLB DONE!\n");
        if (region->len_bytes > threshold) {
            write_cr3(p->cr3.val);
            DEBUG("flush TLB DONE!\n");
        } else {
            uint64_t offset = 0;
            while (offset < region->len_bytes) {
                invlpg((addr_t)region->va_start + (addr_t) offset);
                offset = offset + p->chars.granularity;
            }
            DEBUG("virtual address cache from %016lx to %016lx are invalidated\n", 
                region->va_start, 
                region->va_start + region->len_bytes
            );
        }
    } else {
        // TLB shootdown out of the scope
    }
    return 0;
}

int eager_drill_wrapper(nk_aspace_paging_t *p, nk_aspace_region_t *region) {
    /*
        Only to be called if region passed the following check:
            1. alignment and granularity check 
            2. region overlap or other region validnesss check (involved using p->llist_tracker)
            3. region allocation must be eager
    */
    ph_pf_access_t access_type = access_from_region(region);
    uint64_t page_granularity = PAGE_SIZE_4KB;
    int ret = 0;

    int file_mapping = (region->protect.flags & NK_ASPACE_FILE) != 0;
    int anon_mapping = (region->protect.flags & NK_ASPACE_ANON) != 0;

    // Iterate over the page inds and map each page as required.
    for (uint64_t page = 0; page < region->len_bytes / PAGE_SIZE_4KB; page += 1) {
        addr_t vaddr = (addr_t)region->va_start + (page * PAGE_SIZE_4KB);
        addr_t paddr = (addr_t)region->pa_start + (page * PAGE_SIZE_4KB);

        if (anon_mapping || file_mapping) {
            // if we are mapping anonymous memory, or a file, we need to allocate the backing memory here.
            paddr = (addr_t)malloc(PAGE_SIZE_4KB);
            // printk("allocated %p\n", paddr);
            // if it's a file mapping, read the file into the page
            if (file_mapping) {
								// seek?
                nk_fs_read(region->file, (void*)paddr, PAGE_SIZE_4KB);
                uint64_t val = *(uint64_t*)(void*)paddr;
            }
        }


        ret = paging_helper_drill(p->cr3, vaddr, paddr, access_type);

        if (ret < 0) {
            ERROR("Failed to drill at virtual address=%p"
                    " physical adress %p"
                    " and ret code of %d"
                    " page_granularity = %lx\n",
                    vaddr, paddr, ret, page_granularity
            );
            return ret;
        }
    }

    return ret;
}


// The function the aspace abstraction will call when it
// is adding a region to your address space
static int add_region(void *state, nk_aspace_region_t *region)
{
    // add the new node into region_list
    nk_aspace_paging_t *p = (nk_aspace_paging_t *)state;

    DEBUG("adding region (va=%016lx pa=%016lx len=%lx) to address space %s\n", region->va_start, region->pa_start, region->len_bytes,ASPACE_NAME(p));

    ASPACE_LOCK_CONF;

    ASPACE_LOCK(p);

    // WRITE ME!!
    
    // first you should sanity check the region to be sure it doesn't overlap
    // an existing region, and then place it into your region data structure
    int ret = 0;

    ret = region_align_check(p, region);
    if (ret < 0) {
        ASPACE_UNLOCK(p);
        return ret;
    }

    // sanity check to be sure it doesn't overlap an existing region...
    nk_aspace_region_t * overlap_ptr = mm_check_overlap(p->llist_tracker, region);
    if (overlap_ptr) {
        DEBUG("region Overlapping:\n"
                "\t(va=%016lx pa=%016lx len=%lx, prot=%lx) \n"
                "\t(va=%016lx pa=%016lx len=%lx, prot=%lx) \n", 
            region->va_start, region->pa_start, region->len_bytes, region->protect.flags,
            overlap_ptr->va_start, overlap_ptr->pa_start, overlap_ptr->len_bytes, overlap_ptr->protect.flags
        );
        ASPACE_UNLOCK(p);
        return -1;
    }

    // NOTE: you MUST create a new nk_aspace_region_t to store in your data structure
    // and you MAY NOT store the region pointer in your data structure. There is no
    // promise that data at the region pointer will not be modified after this function
    // returns

    if (region->protect.flags & NK_ASPACE_EAGER) {
	
	// an eager region means that we need to build all the corresponding
	// page table entries right now, before we return

	// DRILL THE PAGE TABLES HERE
        ret = eager_drill_wrapper(p, region);
        if (ret < 0) {
            ASPACE_UNLOCK(p);
            return ret;
        }

    }

    /**
     *  add and print 
     * */
    mm_insert(p->llist_tracker, region);
    // DEBUG("after mm_insert\n");
    mm_show(p->llist_tracker);


    // if we are editing the current address space of this cpu, then we
    // might need to flush the TLB here.   We can do that with a cr3 write
    // like: write_cr3(p->cr3.val);
    clear_cache(p,region, THRESH);
    // if this aspace is active on a different cpu, we might need to do
    // a TLB shootdown here (out of scope of class)
    // a TLB shootdown is an interrupt to a remote CPU whose handler
    // flushes the TLB

    ASPACE_UNLOCK(p);
    
    return 0;
}

// The function the aspace abstraction will call when it
// is removing a region from your address space
static int remove_region(void *state, nk_aspace_region_t *region)
{
    nk_aspace_paging_t *p = (nk_aspace_paging_t *)state;

    DEBUG("removing region (va=%016lx pa=%016lx len=%lx) from address space %s\n", region->va_start, region->pa_start, region->len_bytes,ASPACE_NAME(p));

    ASPACE_LOCK_CONF;

    ASPACE_LOCK(p);

    // WRITE ME!
    
    // first, find the region in your data structure
    // it had better exist and be identical.

    // next, remove the region from your data structure
    // if (NK_ASPACE_GET_PIN(region->protect.flags)) {
    //     ERROR("Cannot remove pinned region"REGION_FORMAT"\n", REGION(region));
    //     ASPACE_UNLOCK(p);
    //     return -1;
    // }

    uint8_t check_flag = VA_CHECK | PA_CHECK | LEN_CHECK | PROTECT_CHECK;
    int remove_failed = mm_remove(p->llist_tracker, region, check_flag);

    int file_mapping = (region->protect.flags & NK_ASPACE_FILE) != 0;
    int anon_mapping = (region->protect.flags & NK_ASPACE_ANON) != 0;

    if (remove_failed) {
        DEBUG("region to remove \
            (va=%016lx pa=%016lx len=%lx, prot=%lx) not FOUND\n", 
            region->va_start, 
            region->pa_start, 
            region->len_bytes,
            region->protect.flags
        );
        ASPACE_UNLOCK(p);
        return -1;
    }

    // next, remove all corresponding page table entries that exist
    ph_pf_access_t access_type = access_from_region(region);
    uint64_t offset = 0;
    
    while (offset < region->len_bytes){
        uint64_t *entry;
        addr_t virtaddr = (addr_t) region->va_start + (addr_t) offset;
        int ret = paging_helper_walk(p->cr3, virtaddr, access_type, &entry);
       
        if (ret == 0) {
            ((ph_pte_t *) entry)->present = 0;
            offset = offset + PAGE_SIZE_4KB;

            // we need to free memory if the region was anonymous or file-backed.
            addr_t paddr = ((ph_pte_t *) entry)->page_base << 12;
            if (anon_mapping || file_mapping) free((void*)paddr);
        } 
        else {
            panic("unexpected return from page walking = %d\n", ret);
        }
    }
    
    
    // next, if we are editing the current address space of this cpu,
    // we need to either invalidate individual pages using invlpg()
    // or do a full TLB flush with a write to cr3.
    clear_cache(p, region, THRESH);

    ASPACE_UNLOCK(p);

    return 0;

}
   
// The function the aspace abstraction will call when it
// is changing the protections of an existing region
static int protect_region(void *state, nk_aspace_region_t *region, nk_aspace_protection_t *prot)
{
    nk_aspace_paging_t *p = (nk_aspace_paging_t *)state;

    DEBUG("protecting region (va=%016lx pa=%016lx len=%lx) from address space %s\n", region->va_start, region->pa_start, region->len_bytes,ASPACE_NAME(p));

    DEBUG("protecting region" 
            "(va=%016lx pa=%016lx len=%lx, prot=%lx)" 
            "from address space %s"
            "to new protection = %lx\n", 
            region->va_start, region->pa_start, region->len_bytes, region->protect.flags,
            ASPACE_NAME(p),
            prot->flags
    );

    DEBUG("Old protection details:" 
        "(read=%d write=%d exec=%d pin=%d kern=%d swap=%d eager=%d)\n",
        NK_ASPACE_GET_READ(region->protect.flags),
        NK_ASPACE_GET_WRITE(region->protect.flags),
        NK_ASPACE_GET_EXEC(region->protect.flags),
        NK_ASPACE_GET_PIN(region->protect.flags), 
        NK_ASPACE_GET_KERN(region->protect.flags), 
        NK_ASPACE_GET_SWAP(region->protect.flags), 
        NK_ASPACE_GET_EAGER(region->protect.flags)
    );

    DEBUG("new protection details:" 
        "(read=%d write=%d exec=%d pin=%d kern=%d swap=%d eager=%d)\n",
        NK_ASPACE_GET_READ(prot->flags),
        NK_ASPACE_GET_WRITE(prot->flags),
        NK_ASPACE_GET_EXEC(prot->flags),
        NK_ASPACE_GET_PIN(prot->flags), 
        NK_ASPACE_GET_KERN(prot->flags), 
        NK_ASPACE_GET_SWAP(prot->flags), 
        NK_ASPACE_GET_EAGER(prot->flags)
    );

    ASPACE_LOCK_CONF;

    ASPACE_LOCK(p);

    // WRITE ME!    
    
    int ret = region_align_check(p, region);
    if (ret < 0) {
        ASPACE_UNLOCK(p);
        return ret;
    }
    
    nk_aspace_region_t new_prot_wrapper = *region;
    new_prot_wrapper.protect = *prot;
    // first, find the region in your data structure
    // it had better exist and be identical except for protections
    // next, update the region protections from your data structure

    /**
     *  update_region function does check and update in one shot
     * */
    uint8_t check_flag = VA_CHECK | LEN_CHECK | PA_CHECK;
    nk_aspace_region_t* reg_ptr = mm_update_region(p->llist_tracker, region, &new_prot_wrapper, check_flag);
    
    if (reg_ptr == NULL) {
        DEBUG("region to update protect \
             (va=%016lx pa=%016lx len=%lx, prot=%lx) not FOUND", 
            region->va_start, 
            region->pa_start, 
            region->len_bytes,
            region->protect.flags
        );
        ASPACE_UNLOCK(p);
        return -1;
    }

    
    // next, update all corresponding page table entries that exist
    ph_pf_access_t access_type = access_from_region(region);
    ph_pf_access_t new_access = access_from_region(reg_ptr);

    // next, update all corresponding page table entries that exist
    /**
     *  if eager just redrill everything
     *      drill function will update the protections
     * */
    if (NK_ASPACE_GET_EAGER(reg_ptr->protect.flags)) {
        ret = eager_drill_wrapper(p, reg_ptr);
        if (ret < 0) {
            ASPACE_UNLOCK(p);
            return ret;
        }
    } else if (access_type.val != new_access.val) {
        
        /**
         *  if not eager but need to update
         *      invalid all entries.
         *      Exception handler will update the protection 
         * */
        uint64_t offset = 0;

        while (offset < reg_ptr->len_bytes){
            uint64_t *entry;
            addr_t virtaddr = (addr_t) region->va_start + (addr_t) offset;
            int ret = paging_helper_walk(p->cr3, virtaddr, access_type, &entry);
            
            if (ret == 0) {
                ((ph_pte_t *) entry)->present = 0;
                perm_set(entry, new_access);
                
            } 
            
            offset = offset + PAGE_SIZE_4KB;
            
        }
    }


    // next, if we are editing the current address space of this cpu,
    // we need to either invalidate individual pages using invlpg()
    // or do a full TLB flush with a write to cr3.
    clear_cache(p, reg_ptr, THRESH);

    // DEBUG("protection done!\n");
    ASPACE_UNLOCK(p);

    return 0;
}

static int move_region(void *state, nk_aspace_region_t *cur_region, nk_aspace_region_t *new_region)
{
    nk_aspace_paging_t *p = (nk_aspace_paging_t *)state;

    DEBUG("moving region (va=%016lx pa=%016lx len=%lx) in address space %s to (va=%016lx pa=%016lx len=%lx)\n", cur_region->va_start, cur_region->pa_start, cur_region->len_bytes,ASPACE_NAME(p),new_region->va_start,new_region->pa_start,new_region->len_bytes);

    ASPACE_LOCK_CONF;

    ASPACE_LOCK(p);

    // WRITE ME!
    
    int ret = region_align_check(p, cur_region);
    if (ret < 0) {
        ASPACE_UNLOCK(p);
        return ret;
    }
    
    ret = region_align_check(p, new_region);
    if (ret < 0) {
        ASPACE_UNLOCK(p);
        return ret;
    }


    /**
     *  I here merge the checking steps and update step together.
     *      the mm_update_region does check and then update
     * */
    // first, find the region in your data structure
    // it had better exist and be identical except for the physical addresses
    uint8_t check_flag = VA_CHECK | LEN_CHECK | PROTECT_CHECK;
    int reg_eq = region_equal(cur_region, new_region, check_flag);
    if (!reg_eq) {
        DEBUG("regions differ in attributes other than physical address!\n");
        ASPACE_UNLOCK(p);
        return -1;
    }

    if (NK_ASPACE_GET_PIN(cur_region->protect.flags)) {
        ERROR("Cannot move pinned region"REGION_FORMAT"\n", REGION(cur_region));
        ASPACE_UNLOCK(p);
        return -1;
    }

    // next, update the region in your data structure
    nk_aspace_region_t* reg_ptr = mm_update_region(
                                    p->llist_tracker, 
                                    cur_region,
                                    new_region,
                                    check_flag
                                );

    if (!reg_ptr) {
        DEBUG(
            "region to update"
            "(va=%016lx pa=%016lx len=%lx, prot=%lx) not FOUND", 
            cur_region->va_start, 
            cur_region->pa_start, 
            cur_region->len_bytes,
            cur_region->protect.flags
        );
        ASPACE_UNLOCK(p);
        return -1;
    }


    // you can assume that the caller has done the work of copying the memory
    // contents to the new physical memory

    // next, update all corresponding page table entries that exist
    ph_pf_access_t access_type = access_from_region(cur_region);
    uint64_t offset = 0;

    // invalidate pages for cur_region
    while (offset < cur_region->len_bytes){
        uint64_t *entry;
        addr_t virtaddr = (addr_t) cur_region->va_start + (addr_t) offset;
        int ret = paging_helper_walk(p->cr3, virtaddr, access_type, &entry);
       
        if (ret == 0) {
            ((ph_pte_t *) entry)->present = 0;
            offset = offset + PAGE_SIZE_4KB;
        } 
        else {
            panic("unexpected return from page walking = %d\n", ret);
        }
    }
    


    if (NK_ASPACE_GET_EAGER(new_region->protect.flags)) {
	// an eager region means that we need to build all the corresponding
	// page table entries right now, before we return
	// DRILL THE PAGE TABLES HERE
        ret = eager_drill_wrapper(p, new_region);
        if (ret < 0) {
            ASPACE_UNLOCK(p);
            return ret;
        }
    } else {
        // nothing to do for uneager region
    }
    

    // next, if we are editing the current address space of this cpu,
    // we need to either invalidate individual pages using invlpg()
    // or do a full TLB flush with a write to cr3.
    clear_cache(p, cur_region, THRESH);
    clear_cache(p, new_region, THRESH);

    // OPTIONAL ADVANCED VERSION: allow for splitting the region - if cur_region
    // is a subset of some region, then split that region, and only move
    // the affected addresses.   The granularity of this is that reported
    // in the aspace characteristics (i.e., page granularity here).

    ASPACE_UNLOCK(p);

    return 0;
}


// called by the address space abstraction when it is switching away from
// the noted address space.   This is part of the thread context switch.
// do you care?
static int switch_from(void *state)
{
    struct nk_aspace_paging *p = (struct nk_aspace_paging *)state;
    struct nk_thread *thread = get_cur_thread();
    
    DEBUG("switching out address space %s from thread %d (%s)\n",ASPACE_NAME(p), thread->tid, THREAD_NAME(thread));

    return 0;
}

// called by the address space abstraction when it is switching to the
// noted address space.  This is part of the thread context switch.
static int switch_to(void *state)
{
    nk_aspace_paging_t *p = (nk_aspace_paging_t *)state;
    struct nk_thread *thread = get_cur_thread();
    
    DEBUG("switching in address space %s from thread %d (%s)\n", ASPACE_NAME(p),thread->tid,THREAD_NAME(thread));
    DEBUG("new cr3=%p, old=%p\n", p->cr3.val, read_cr3());

    
    // Here you will need to install your page table hierarchy
    // first point CR3 to it
    write_cr3(p->cr3.val);

    // next make sure the interpretation bits are set in cr4
    uint64_t cr4 = read_cr4();
    cr4 &= ~CR4_MASK;
    cr4 |= p->cr4;
    write_cr4(cr4);
    
    return 0;
}

// called by the address space abstraction when a page fault or a
// general protection fault is encountered in the context of the
// current thread
//
// exp points to the hardware interrupt frame on the stack
// vec indicates which vector happened
//
static int exception(void *state, excp_entry_t *exp, excp_vec_t vec)
{
    nk_aspace_paging_t *p = (nk_aspace_paging_t *)state;
    struct nk_thread *thread = get_cur_thread();
    
    // DEBUG("exception 0x%x for address space %s in context of thread %d (%s)\n",vec,ASPACE_NAME(p),thread->tid,THREAD_NAME(thread));
    
    if (vec==GP_EXCP) {
	ERROR("general protection fault encountered.... uh...\n");
	ERROR("i have seen things that you people would not believe.\n");
	// panic("general protection fault delivered to paging subsystem\n");
    // nk_thread_exit(NULL);
	return -1; // will never happen
    }

    if (vec!=PF_EXCP) {
	ERROR("Unknown exception %d delivered to paging subsystem\n",vec);
	panic("Unknown exception delivered to paging subsystem\n");
	return -1; // will never happen
    }
    
    // It must be a page fault
    
    // find out what the fault address and the fault reason
    uint64_t virtaddr = read_cr2();
    ph_pf_error_t  error;
    error.val = exp->error_code;
    
    // DEBUG("Page fault at virt_addr = %llx, error = %llx\n", virtaddr, error.val);
    
    // DEBUG("Page fault at error.present = %x, "
    //         "error.write = %x " 
    //         "error.user = %x " 
    //         "error.rsvd_access = %x " 
    //         "error.ifetch = %x \n", 
    //         error.present,
    //         error.write,
    //         error.user,
    //         error.rsvd_access,
    //         error.ifetch    
    // );

    
    ASPACE_LOCK_CONF;
    
    ASPACE_LOCK(p);

    //
    // WRITE ME!
    //
    
    // Now find the region corresponding to this address
    // if there is no such region, this is an unfixable fault
    //   (if this is a user thread, we now would signal it or kill it, but there are no user threads in Nautilus)
    //   if it's a kernel thread, the kernel should panic
    //   if it's within an interrupt handler, the kernel should panic
    nk_aspace_region_t * region = mm_find_reg_at_addr(p->llist_tracker, (addr_t) virtaddr);
    if (region == NULL) {
        panic("Page Fault at %p, but no matching region found\n", virtaddr);
        ASPACE_UNLOCK(p);
        return -1;
    }

    // Is the problem that the page table entry is not present?
    // if so, drill the entry and then return from the function
    // so the faulting instruction can try agai
    //    This is the lazy construction of the page table entries

    int ret;
    ph_pf_access_t access_type = access_from_region(region);

    if(!error.present) {
        addr_t vaddr, paddr, vaddr_try, paddr_try;
        uint64_t remained, remained_try, page_granularity;
        
        addr_t va_start = (addr_t) region->va_start;
        addr_t pa_start = (addr_t) region->pa_start;

        

        addr_t vaddr_4KB_align = virtaddr - ADDR_TO_OFFSET_4KB(virtaddr);

        vaddr = vaddr_4KB_align;
        paddr = vaddr - va_start + pa_start;
        remained = region->len_bytes - (vaddr - va_start);
        page_granularity = PAGE_SIZE_4KB;

        ret = paging_helper_drill(p->cr3, vaddr, paddr, access_type);

        if (ret < 0) {
            ERROR("Failed to drill at virtual address=%p"
                    " physical adress %p"
                    " and ret code of %d"
                    " page_granularity = %lx\n",
                    vaddr, paddr, ret, page_granularity
            );
            ASPACE_UNLOCK(p);
            return ret;
        }
        
    } else {
        
        // Assuming the page table entry is present, check the region's
        // protections and compare to the error code

        // if the region has insufficient permissions for the request,
        // then this is an unfixable fault
        //   if this is a user thread, we now would signal it or kill it
        //   if it's a kernel thread, the kernel should panic
        //   if it's within an interrupt handler, the kernel should panic

        int ok = (access_type.write >= error.write) && 
                    (access_type.user>= error.user) && 
                    (access_type.ifetch >= error.ifetch);

        DEBUG(
            "region.protect.write=%d, error.write=%d\n" 
            "region.protect.user=%d, error.user=%d\n" 
            "region.protect.ifetch=%d, error.ifetch=%d\n",
            access_type.write, error.write,
            access_type.user, error.user,
            access_type.ifetch, error.ifetch
        );

        if(ok){
            ASPACE_UNLOCK(p);
            panic("weird Page fault with permission ok and page present\n");
            return 0;
        } else{
            ASPACE_UNLOCK(p);
            nk_vc_printf("Permission not allowed\n");
            #ifdef NAUT_CONFIG_ENABLE_USERSPACE
                return set_pending_signal();
            #else
                return -1;
            #endif
        }
    }

    // if the region has insufficient permissions for the request,
    // then this is an unfixable fault
    //   (if this is a user thread, we now would signal it or kill it, but there are no user threads in Nautilus)
    //   if it's a kernel thread, the kernel should panic
    //   if it's within an interrupt handler, the kernel should panic
    
    // DEBUG("Done: Paging Exception handler!\n");
    ASPACE_UNLOCK(p);
    
    return 0;
}
    
// called by the address space abstraction when it wants you
// to print out info about the address space.  detailed is
// nonzero if it wants a detailed output.  Use the nk_vc_printf()
// function to print here
static int print(void *state, int detailed)
{
    nk_aspace_paging_t *p = (nk_aspace_paging_t *)state;
    struct nk_thread *thread = get_cur_thread();
    

    // basic info
    nk_vc_printf("%s: paging address space [granularity 0x%lx alignment 0x%lx]\n"
		 "   CR3:    %016lx  CR4m: %016lx\n",
		 ASPACE_NAME(p), p->chars.granularity, p->chars.alignment, p->cr3.val, p->cr4);

    if (detailed) {
	// print region set data structure here

	// perhaps print out all the page tables here...
    }

    return 0;
}    

//
// This structure binds together your interface functions
// with the interface definition of the address space abstraction
// it will be used later in registering an address space
//
static nk_aspace_interface_t paging_interface = {
    .destroy = destroy,
    .add_thread = add_thread,
    .remove_thread = remove_thread,
    .add_region = add_region,
    .remove_region = remove_region,
    .protect_region = protect_region,
    .move_region = move_region,
    .switch_from = switch_from,
    .switch_to = switch_to,
    .exception = exception,
    .print = print
};


//
// The address space abstraction invokes this function when
// someone asks about your implementations characterstics
//
static int   get_characteristics(nk_aspace_characteristics_t *c)
{
    // you must support 4KB page granularity and alignment
    c->granularity = c->alignment = PAGE_SIZE_4KB;
    // you must support anonymous mappings, as well as file mappings
    c->capabilities = NK_ASPACE_CAP_ANON | NK_ASPACE_CAP_FILE;
    
    return 0;
}


//
// The address space abstraction invokes this function when
// someone wants to create a new paging address space with the given
// name and characteristics
//
static struct nk_aspace * create(char *name, nk_aspace_characteristics_t *c)
{
    struct naut_info *info = nk_get_nautilus_info();
    nk_aspace_paging_t *p;
    
    p = malloc(sizeof(*p));
    
    if (!p) {
	ERROR("cannot allocate paging aspace %s\n",name);
	return 0;
    }
  
    memset(p,0,sizeof(*p));
    
    spinlock_init(&p->lock);

    // initialize your region set data structure here!
    p->llist_tracker = mm_llist_create();
    p->chars = *c;

    // create an initial top-level page table (PML4)
    if(paging_helper_create(&(p->cr3)) == -1){
	ERROR("unable create aspace cr3 in address space %s\n", name);
    }

    // note also the cr4 bits you should maintain
    p->cr4 = nk_paging_default_cr4() & CR4_MASK;
    
    

    // if we supported address spaces other than long mode
    // we would also manage the EFER register here

    // Register your new paging address space with the address space
    // space abstraction
    // the registration process returns a pointer to the abstract
    // address space that the rest of the system will use
    p->aspace = nk_aspace_register(name,
				   // we want both page faults and general protection faults
				   NK_ASPACE_HOOK_PF | NK_ASPACE_HOOK_GPF,
				   // our interface functions (see above)
				   &paging_interface,
				   // our state, which will be passed back
				   // whenever any of our interface functiosn are used
				   p);
    
    if (!p->aspace) {
	ERROR("Unable to register paging address space %s\n",name);
	return 0;
    }
    
    DEBUG("paging address space %s configured and initialized (returning %p)\n", name, p->aspace);
    
    // you are returning
    return p->aspace; 
}

//
// This structure binds together the interface functions of our
// implementation with the relevant interface definition
static nk_aspace_impl_t paging = {
				.impl_name = "paging",
				.get_characteristics = get_characteristics,
				.create = create,
};


// this does linker magic to populate a table of address space
// implementations by including this implementation
nk_aspace_register_impl(paging);


