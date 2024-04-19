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
#ifndef __GICV2_H__
#define __GICV2_H__

#include<nautilus/nautilus.h>
#include<nautilus/naut_types.h>

#ifndef NAUT_CONFIG_DEBUG_GIC
#undef DEBUG_PRINT
#define DEBUG_PRINT(fmt, args...)
#endif

#define GIC_PRINT(fmt, args...) printk("[GICv2] " fmt, ##args)
#define GIC_DEBUG(fmt, args...) DEBUG_PRINT("[GICv2] " fmt, ##args)
#define GIC_ERROR(fmt, args...) ERROR_PRINT("[GICv2] " fmt, ##args)
#define GIC_WARN(fmt, args...) WARN_PRINT("[GICv2] " fmt, ##args)

struct gicv2 {

  uint64_t dist_base;
  uint64_t cpu_base;

  nk_irq_t irq_base;
  struct nk_irq_desc *sgi_descs;
  struct nk_irq_desc *ppi_descs;
  struct nk_irq_desc *spi_descs;
  nk_irq_t special_irq_base;
  struct nk_irq_desc *special_descs;

  uint32_t max_spi;
  uint32_t num_cpus;

  uint8_t security_ext;

#ifdef NAUT_CONFIG_GIC_VERSION_2M
  struct gicv2m_msi_frame *msi_frame;
#endif
};

int gicv2_init(void);
int gicv2_percpu_init(void);

#endif
