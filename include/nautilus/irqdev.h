/*
 * This file is part of the Nautilus AeroKernel developed
 * by the Constellation, Interweaving, Hobbes, and V3VEE
 * Projects with funding from the United States National
 * Science Foundation and the Department of Energy.
 *
 * The V3VEE Project is a joint project between Northwestern University
 * and the University of New Mexico.  The Hobbes Project is a collaboration
 * led by Sandia National Laboratories that includes several national
 * laboratories and universities. The Interweaving Project is a
 * joint project between Northwestern University and Illinois Institute
 * of Technology.   The Constellation Project is a joint project
 * between Northwestern University, Carnegie Mellon University,
 * and Illinois Institute of Technology.
 * You can find out more at:
 * http://www.v3vee.org
 * http://xstack.sandia.gov/hobbes
 * http://interweaving.org
 * http://constellation-project.net
 *
 * Copyright (c) 2023, Kevin Hayes <kjhayes@u.northwestern.edu>
 * Copyright (c) 2023, The V3VEE Project  <http://www.v3vee.org>
 *                     The Hobbes Project <http://xstack.sandia.gov/hobbes>
 * All rights reserved.
 *
 * Authors: Kevin Hayes <kjhayes@u.northwestern.edu>
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "LICENSE.txt".
 */

#ifndef __IRQ_DEV_H__
#define __IRQ_DEV_H__

#include<nautilus/nautilus.h>
#include<nautilus/smp.h>
#include<nautilus/dev.h>
#include<nautilus/dev_info.h>

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

  // Map hwirq to global IRQ
  int (*revmap)(void *state, nk_hwirq_t hwirq, nk_irq_t *irq);

  // Firmware Interrupt Specifier Translation
  int(*translate)(void *state, nk_dev_info_type_t type, const void *raw_irq, nk_hwirq_t *out_hwirq);

  // IPI's
  int(*send_ipi)(void *state, nk_hwirq_t hwirq, cpu_id_t cpu);
  int(*broadcast_ipi)(void *state, nk_hwirq_t hwirq);

  // MSI (-X) Support
  int (*msi_addr)(void *state, nk_hwirq_t hwirq, void **addr);
  int (*msi_msg)(void *state, nk_hwirq_t hwirq, uint16_t *msg);

  int (*msi_block_size)(void *state, nk_hwirq_t hwirq, size_t *size);
  int (*msi_index_block)(void *state, nk_hwirq_t hwirq, size_t index, nk_hwirq_t *out);

  int (*msi_x_addr)(void *state, nk_hwirq_t hwirq, void **addr);
  int (*msi_x_msg)(void *state, nk_hwirq_t hwirq, uint32_t *msg);
};

struct nk_irq_dev {
  // Must be first
  struct nk_dev dev;
};

/*
 * Device Interface to the rest of the Kernel
 */

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

#define IRQ_IPI_SUCCESS       0
#define IRQ_IPI_ERROR_IRQ_NO  (1<<1) // Error with the nk_hwirq_t being requested
#define IRQ_IPI_ERROR_CPUID   (1<<2) // Error with the cpu_id_t being requested
#define IRQ_IPI_ERROR_UNKNOWN (1<<3) // Unknown Cause 
int nk_irq_dev_send_ipi(struct nk_irq_dev *d, nk_hwirq_t hwirq, cpu_id_t cpu);
// Sends the IPI to all processors other than the current processor
int nk_irq_dev_broadcast_ipi(struct nk_irq_dev *d, nk_hwirq_t hwirq);

#define IRQ_MSI_SUCCESS       0
#define IRQ_MSI_UNSUPPORTED   (1<<0)
#define IRQ_MSI_ERROR_UNKNOWN (1<<1)
int nk_irq_dev_msi_addr(struct nk_irq_dev *d, nk_hwirq_t hwirq, void **addr);
int nk_irq_dev_msi_msg(struct nk_irq_dev *d, nk_hwirq_t hwirq, uint16_t *msg);
int nk_irq_dev_msi_x_addr(struct nk_irq_dev *d, nk_hwirq_t hwirq, void **addr);
int nk_irq_dev_msi_x_msg(struct nk_irq_dev *d, nk_hwirq_t hwirq, uint32_t *msg);

int nk_irq_dev_msi_block_size(struct nk_irq_dev *dev, nk_hwirq_t, size_t *num);
int nk_irq_dev_msi_index_block(struct nk_irq_dev *dev, nk_hwirq_t, size_t index, nk_hwirq_t *out);

/*
 * Maps from IRQ device local interrupt numbers to global nk_irq_t
 */
int nk_irq_dev_revmap(struct nk_irq_dev *d, nk_hwirq_t hwirq, nk_irq_t *out_irq);
int nk_irq_dev_translate(struct nk_irq_dev *d, nk_dev_info_type_t type, const void *raw_irq, nk_hwirq_t *out_hwirq);

int nk_assign_cpu_irq_dev(struct nk_irq_dev *dev, cpu_id_t cpuid);
int nk_assign_all_cpus_irq_dev(struct nk_irq_dev *dev);

#endif
