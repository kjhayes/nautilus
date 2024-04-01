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
#include <nautilus/smp.h>
#include <nautilus/paging.h>
#include <nautilus/cpu.h>
#include <nautilus/naut_assert.h>
#include <nautilus/thread.h>
#include <nautilus/queue.h>
#include <nautilus/idle.h>
#include <nautilus/atomic.h>
#include <nautilus/numa.h>
#include <nautilus/mm.h>
#include <nautilus/percpu.h>
#include <nautilus/interrupt.h>
#include <nautilus/xcall.h>

#ifdef NAUT_CONFIG_ALLOCS
#include <nautilus/alloc.h>
#endif

#ifdef NAUT_CONFIG_ASPACES
#include <nautilus/aspace.h>
#endif

#ifdef NAUT_CONFIG_FIBER_ENABLE
#include <nautilus/fiber.h>
#endif

#ifdef NAUT_CONFIG_ENABLE_REMOTE_DEBUGGING 
#include <nautilus/gdb-stub.h>
#endif

#ifdef NAUT_CONFIG_CACHEPART
#include <nautilus/cachepart.h>
#endif

#ifdef NAUT_CONFIG_LINUX_SYSCALLS
#include <nautilus/syscalls/kernel.h>
#endif


#include <arch/x64/irq.h>

#ifndef NAUT_CONFIG_DEBUG_XCALL
#undef DEBUG_PRINT
#define DEBUG_PRINT(fmt, args...)
#endif
#define XCALL_PRINT(fmt, args...) printk("XCALL: " fmt, ##args)
#define XCALL_DEBUG(fmt, args...) DEBUG_PRINT("XCALL: " fmt, ##args)

int
smp_setup_xcall_bsp (struct cpu * core)
{
    XCALL_PRINT("Setting up cross-core IPI event queue\n");
    smp_xcall_init_queue(core);

    nk_irq_t xcall_irq = arch_xcall_irq();
    if(xcall_irq == NK_NULL_IRQ) {
        ERROR_PRINT("arch_xcall_irq returned NK_NULL_IRQ during xcall initialization!\n");
        return -1;
    }

    if (nk_irq_add_handler(xcall_irq, xcall_handler, NULL) != 0) {
        ERROR_PRINT("Could not assign interrupt handler for XCALL on core %u\n", core->id);
        return -1;
    }

    return 0;
}

int
smp_xcall_init_queue (struct cpu * core)
{
    core->xcall_q = nk_queue_create();
    if (!core->xcall_q) {
        ERROR_PRINT("Could not allocate xcall queue on cpu %u\n", core->id);
        return -1;
    }

    return 0;
}

static void
init_xcall (struct nk_xcall * x, void * arg, nk_xcall_func_t fun, int wait)
{
    x->data       = arg;
    x->fun        = fun;
    x->xcall_done = 0;
    x->has_waiter = wait;
}


static inline void
wait_xcall (struct nk_xcall * x)
{

    while (atomic_cmpswap(x->xcall_done, 1, 0) != 1) {
        arch_relax();
    }
}


static inline void
mark_xcall_done (struct nk_xcall * x) 
{
    atomic_cmpswap(x->xcall_done, 0, 1);
}

int
xcall_handler (struct nk_irq_action * action, struct nk_regs * regs, void *state) 
{
    nk_queue_t * xcq = per_cpu_get(xcall_q); 
    struct nk_xcall * x = NULL;
    nk_queue_entry_t * elm = NULL;

    if (!xcq) {
        ERROR_PRINT("Badness: no xcall queue on core %u\n", my_cpu_id());
        goto out_err;
    }

    elm = nk_dequeue_first_atomic(xcq);
    x = container_of(elm, struct nk_xcall, xcall_node);
    if (!x) {
        ERROR_PRINT("No XCALL request found on core %u\n", my_cpu_id());
        goto out_err;
    }

    if (x->fun) {

        // we ack the IPI before calling the handler funciton,
        // because it may end up blocking (e.g. core barrier)
        //
        // TODO: 
        // This doesn't work anymore, we can still block because EOI is done after returning from this function.
        // Need to re-add a mechanism to signal EOI early (or turn this into a top half handler) - KJH
        IRQ_HANDLER_END(); 

        x->fun(x->data);

        /* we need to notify the waiter we're done */
        if (x->has_waiter) {
            mark_xcall_done(x);
        }

    } else {
        ERROR_PRINT("No XCALL function found on core %u\n", my_cpu_id());
        goto out_err;
    }


    return 0;

out_err:
    IRQ_HANDLER_END();
    return -1;
}


/* 
 * smp_xcall
 *
 * initiate cross-core call. 
 * 
 * @cpu_id: the cpu to execute the call on
 * @fun: the function to invoke
 * @arg: the argument to the function
 * @wait: this function should block until the reciever finishes
 *        executing the function
 *
 */
int
smp_xcall (cpu_id_t cpu_id, 
           nk_xcall_func_t fun,
           void * arg,
           uint8_t wait)
{
    struct sys_info * sys = per_cpu_get(system);
    nk_queue_t * xcq  = NULL;
    struct nk_xcall x;
    uint8_t flags;

    XCALL_DEBUG("Initiating SMP XCALL from core %u to core %u\n", my_cpu_id(), cpu_id);

    if (cpu_id > nk_get_num_cpus()) {
        ERROR_PRINT("Attempt to execute xcall on invalid cpu (%u)\n", cpu_id);
        return -1;
    }

    if (cpu_id == my_cpu_id()) {

        flags = irq_disable_save();
        fun(arg);
        irq_enable_restore(flags);

    } else {
        struct nk_xcall * xc = &x;
        nk_irq_t xcall_irq = arch_xcall_irq();
        if(xcall_irq == NK_NULL_IRQ) {
            ERROR_PRINT("Attempt by cpu %u to initiate xcall but arch_xcall_irq() == NK_NULL_IRQ!\n", my_cpu_id());
            return -1;
        }

        if (!wait) {
            xc = &(sys->cpus[cpu_id]->xcall_nowait_info);
        }

        init_xcall(xc, arg, fun, wait);

        xcq = sys->cpus[cpu_id]->xcall_q;
        if (!xcq) {
            ERROR_PRINT("Attempt by cpu %u to initiate xcall on invalid xcall queue (for cpu %u)\n", 
                        my_cpu_id(),
                        cpu_id);
            return -1;
        }

        flags = irq_disable_save();

        if (!nk_queue_empty_atomic(xcq)) {
            ERROR_PRINT("XCALL queue for core %u is not empty, bailing\n", cpu_id);
            irq_enable_restore(flags);
            return -1;
        }

        nk_enqueue_entry_atomic(xcq, &(xc->xcall_node));

        irq_enable_restore(flags);

        /*
        struct apic_dev * apic = per_cpu_get(apic);
        apic_ipi(apic, sys->cpus[cpu_id]->apic->id, IPI_VEC_XCALL);
        */
        if(nk_send_ipi(xcall_irq, cpu_id)) {
            ERROR_PRINT("Failed to send IPI from cpu %u to cpu %u during xcall!\n", my_cpu_id(), cpu_id);
            return -1;
        }

        if (wait) {
            wait_xcall(xc);
        }

    }

    return 0;
}
