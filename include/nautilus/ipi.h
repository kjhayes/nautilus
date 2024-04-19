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

#ifndef __IPI_H__
#define __IPI_H__

#include<nautilus/interrupt.h>
#include<nautilus/shell.h>
#include<nautilus/timer.h>
#include<nautilus/thread.h>

#ifdef NAUT_CONFIG_IPI_STRESS
int ipi_stress_init(void);
int set_ipi_stress(cpu_id_t from, cpu_id_t to, nk_irq_t ipi, uint64_t delay);
int clear_ipi_stress(cpu_id_t stressor, uint64_t *num_sent);

#define STRESSED_IPI 15

static inline cpu_id_t begin_ipi_stress_test(uint64_t delay) 
{
  preempt_disable();

  printk("Running on CPU (%u)\n", my_cpu_id());
  cpu_id_t spammer;
  int spamming = 1;
  if(my_cpu_id() + 1 < nk_get_num_cpus()) {
    spammer = my_cpu_id() + 1;
  } else if(my_cpu_id() > 0) {
    spammer = my_cpu_id() - 1;
  } else {
    printk("Cannot stress IPI's on a single core system!\n");
    spamming = 0;
  }

  if(spamming) {
    printk("Setting CPU (%u) to spam IPI (%u) with delay (%u)\n", spammer, STRESSED_IPI, delay);

    struct nk_irq_desc *desc = nk_irq_to_desc(STRESSED_IPI);
    desc->triggered_count = 0;
    set_ipi_stress(spammer, my_cpu_id(), STRESSED_IPI, delay);
  }

  return spammer;
}
static inline void end_ipi_stress_test(cpu_id_t cpu)
{
  uint64_t num_sent = 0;

  if(clear_ipi_stress(cpu, &num_sent)) {
    printk("Failed to stop IPI stress test!\n");
    return;
  }
  if(num_sent == 0) {
    printk("No IPI's were sent!\n");
    return;
  }

  struct nk_irq_desc *desc = nk_irq_to_desc(STRESSED_IPI);

  uint64_t num_handled = desc->triggered_count;
  double percent_handled = ((double)num_handled / (double)num_sent) * 100.0;
  double percent_lost = 100.0 - percent_handled;

  printk("# IPI's Sent: %llu\n", num_sent);
  printk("# IPI's Handled: %llu\n", num_handled);
  printk("Percent Handled = %3.2llf %%\n", percent_handled);
  printk("Percent Lost = %3.2llf %%\n", percent_lost);

  preempt_enable();
}
#endif

#endif
