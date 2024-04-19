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
#ifndef __PL011_H__
#define __PL011_H__

#include<nautilus/naut_types.h>
#include<nautilus/chardev.h>
#include<nautilus/spinlock.h>

struct pl011_uart {
  struct nk_char_dev *dev;

  void *mmio_base;
  uint64_t clock;

  uint32_t baudrate;

  nk_irq_t irq;

  spinlock_t input_lock;
  spinlock_t output_lock;
};

#ifdef NAUT_CONFIG_PL011_UART_EARLY_OUTPUT
int pl011_uart_pre_vc_init(uint64_t dtb);
#endif

int pl011_uart_init(void);

int pl011_uart_dev_write(void*, uint8_t *src);
int pl011_uart_dev_read(void*, uint8_t *dest);

void pl011_uart_putchar_blocking(struct pl011_uart*, const char c);
void pl011_uart_puts_blocking(struct pl011_uart*, const char *s);

#endif
