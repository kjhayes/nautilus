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

#ifndef __NAUT_GPIO_H__
#define __NAUT_GPIO_H__

#include<nautilus/naut_types.h>

#define NK_GPIO_LOW  0
#define NK_GPIO_HIGH 1

#define NK_GPIO_INACTIVE  0
#define NK_GPIO_ACTIVE    1

#define NK_GPIO_OUTPUT        (1<<0)
#define NK_GPIO_INPUT         (1<<1)
#define NK_GPIO_ACTIVE_HIGH   (1<<2)
#define NK_GPIO_ACTIVE_LOW    (1<<3)
#define NK_GPIO_OPEN_DRAIN    (1<<4)
#define NK_GPIO_OPEN_SOURCE   (1<<5)
#define NK_GPIO_PULL_UP       (1<<6)
#define NK_GPIO_PULL_DOWN     (1<<7)

struct nk_gpio_desc 
{
  const char *name;
  nk_gpio_t num;
  int raw_state;
  int flags;
  int type;

  struct nk_gpio_dev *dev;
};

int nk_gpio_init(void);
int nk_gpio_deinit(void);

int nk_gpio_alloc_pins(uint32_t num, struct nk_gpio_dev *dev, nk_gpio_t *base);
struct nk_gpio_desc *nk_gpio_get_gpio_desc(nk_gpio_t);

int nk_gpio_read_flags(nk_gpio_t num, int *flags);
int nk_gpio_write_flags(nk_gpio_t num, int flags);

// Uses NK_GPIO_ACTIVE or NK_GPIO_INACTIVE
int nk_gpio_write_output(nk_gpio_t num, int val);
int nk_gpio_read_input(nk_gpio_t num, int *val);

// Uses NK_GPIO_LOW or NK_GPIO_HIGH
int nk_gpio_write_output_raw(nk_gpio_t num, int val);
int nk_gpio_read_input_raw(nk_gpio_t num, int *val);

#endif
