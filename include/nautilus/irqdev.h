#ifndef __IRQ_DEV_H__
#define __IRQ_DEV_H__

#include<nautilus/nautilus.h>
#include<nautilus/smp.h>
#include<nautilus/dev.h>

struct nk_irq_dev_characteristics {};

struct nk_irq_dev_int {
  // Must be first
  struct nk_dev_int dev_int;

  int (*get_characteristics)(void *state, struct nk_irq_dev_characteristics *c);

  // IRQ Flow Controls
  int (*ack_irq)(void *state, nk_hwirq_t *irq);
  int (*eoi_irq)(void *state, nk_hwirq_t irq);

  //
  int (*enable_irq)(void *state, nk_hwirq_t irq);
  int (*disable_irq)(void *state, nk_hwirq_t irq);
  
  int (*irq_status)(void *state, nk_hwirq_t irq);

  int (*revmap)(void *state, nk_hwirq_t hwirq, nk_irq_t *irq);

  int(*translate)(void *state, nk_dev_info_type_t type, const void *raw_irq, nk_hwirq_t *out_hwirq);

  int(*send_ipi)(void *state, nk_hwirq_t hwirq, cpu_id_t cpu);
  int(*broadcast_ipi)(void *state, nk_hwirq_t hwirq);
};

struct nk_irq_dev {
  // Must be first
  struct nk_dev dev;
};

/*
 * Device Interface to the rest of the Kernel
 */

int nk_irq_dev_init(void);
int nk_irq_dev_deinit(void);

struct nk_irq_dev * nk_irq_dev_register(char *name, uint64_t flags, struct nk_irq_dev_int *inter, void *state);
int nk_irq_dev_unregister(struct nk_irq_dev *);

struct nk_irq_dev * nk_irq_dev_find(char *name);

int nk_irq_dev_get_characteristics(struct nk_irq_dev *d, struct nk_irq_dev_characteristics *c);

// Return Codes for nk_irq_dev_ack
#define IRQ_DEV_ACK_SUCCESS    0
#define IRQ_DEV_ACK_ERR_EOI    1 // ERROR: Return from interrupt but EOI first
#define IRQ_DEV_ACK_ERR_RET    2 // ERROR: Return fromt interrupt without EOI
#define IRQ_DEV_ACK_UNIMPL     3

// Returns which irq was ack'ed in "irq"
int nk_irq_dev_ack(struct nk_irq_dev *d, nk_hwirq_t *irq);

// Return Codes for nk_irq_dev_eoi
#define IRQ_DEV_EOI_SUCCESS    0
#define IRQ_DEV_EOI_ERROR      1
#define IRQ_DEV_EOI_UNIMPL     2
                          
int nk_irq_dev_eoi(struct nk_irq_dev *d, nk_hwirq_t irq);

// 0 -> Success, 1 -> Failure
int nk_irq_dev_enable_irq(struct nk_irq_dev *d, nk_hwirq_t irq);
// 0 -> Success, 1 -> Failure
int nk_irq_dev_disable_irq(struct nk_irq_dev *d, nk_hwirq_t irq);

#define IRQ_STATUS_ERROR   (1<<1) // Error reading the status
#define IRQ_STATUS_ENABLED (1<<2) // Interrupt is enabled (could be signalled but isn't)
#define IRQ_STATUS_PENDING (1<<3) // Interrupt is pending (signalled but not ack'ed)
#define IRQ_STATUS_ACTIVE  (1<<4) // Interrupt is active (between ack and eoi)
int nk_irq_dev_irq_status(struct nk_irq_dev *d, nk_hwirq_t hwirq);

#define IRQ_IPI_ERROR_IRQ_NO  (1<<1) // Error with the nk_hwirq_t being requested
#define IRQ_IPI_ERROR_CPUID   (1<<2) // Error with the cpu_id_t being requested
#define IRQ_IPI_ERROR_UNKNOWN (1<<3) // Unknown Cause 
int nk_irq_dev_send_ipi(struct nk_irq_dev *d, nk_hwirq_t hwirq, cpu_id_t cpu);
// Sends the IPI to all processors other than the current processor
int nk_irq_dev_broadcast_ipi(struct nk_irq_dev *d, nk_hwirq_t hwirq);

/*
 * Maps from IRQ device local interrupt numbers to global nk_irq_t
 */
int nk_irq_dev_revmap(struct nk_irq_dev *d, nk_hwirq_t hwirq, nk_irq_t *out_irq);
int nk_irq_dev_translate(struct nk_irq_dev *d, nk_dev_info_type_t type, const void *raw_irq, nk_hwirq_t *out_hwirq);

int nk_assign_cpu_irq_dev(struct nk_irq_dev *dev, cpu_id_t cpuid);
int nk_assign_all_cpus_irq_dev(struct nk_irq_dev *dev);

#endif
