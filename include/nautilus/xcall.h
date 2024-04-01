#ifndef __NAUT_XCALL_H__
#define __NAUT_XCALL_H__

#include <nautilus/naut_types.h>
#include <nautilus/smp.h>
#include <nautilus/queue.h>

typedef void (*nk_xcall_func_t)(void * arg);

struct nk_xcall {
    nk_queue_entry_t xcall_node;
    void * data;
    nk_xcall_func_t fun;
    uint8_t xcall_done;
    uint8_t has_waiter;
};

// Intialization
int smp_setup_xcall_bsp(struct cpu *bsp);
int smp_xcall_init_queue(struct cpu *ap);

struct nk_irq_action;
struct nk_regs;
int xcall_handler(struct nk_irq_action *, struct nk_regs *, void *);

// User API
int smp_xcall(cpu_id_t cpu_id, nk_xcall_func_t fun, void * arg, uint8_t wait);

#endif
