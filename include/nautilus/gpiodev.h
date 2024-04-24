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

#ifndef __GPIO_DEV_H__
#define __GPIO_DEV_H__

#include<nautilus/dev.h>
#include<nautilus/naut_types.h>

// Linux splits these into two different types of devices entirely
#define NK_GPIO_DEV_TYPE_PINCTRL 0b01
#define NK_GPIO_DEV_TYPE_GPIO    0b10
#define NK_GPIO_DEV_TYPE_DUAL    0b11

struct nk_gpio_dev_characteristics {
  int type : 2;
};

struct nk_gpio_dev_int 
{
  struct nk_dev_int dev_int;

  int (*get_characteristics)(void *state, struct nk_gpio_dev_characteristics *c);

  // GPIO Functions

  int (*write_output)(void *state, nk_gpio_t, int val);
  int (*read_input)(void *state, nk_gpio_t, int* val);

  // NK_GPIO_OUTPUT or NK_GPIO_INPUT
  int (*set_io_type)(void *state, nk_gpio_t, int type);
  int (*get_io_type)(void *state, nk_gpio_t, int *type);
  
  // Pin Ctrl Functions

  // NK_GPIO_ACTIVE_HIGH or NK_GPIO_ACTIVE_LOW
  int (*set_polarity)(void *state, nk_gpio_t, int polarity);
  int (*get_polarity)(void *state, nk_gpio_t, int *polarity);

  // NK_GPIO_OPEN_DRAIN or NK_GPIO_OPEN_SOURCE
  int (*set_open_type)(void *state, nk_gpio_t, int open);
  int (*get_open_type)(void *state, nk_gpio_t, int *open);

  // NK_GPIO_PULL_UP or NK_GPIO_PULL_DOWN
  int (*set_pull_type)(void *state, nk_gpio_t, int pull);
  int (*get_pull_type)(void *state, nk_gpio_t, int *pull);
};

struct nk_gpio_dev 
{
  struct nk_dev dev;
};

struct nk_gpio_dev * nk_gpio_dev_register(char *name, uint64_t flags, struct nk_gpio_dev_int *inter, void *state);
int nk_gpio_dev_unregister(struct nk_gpio_dev*);

/*
 * Device Local Functions
 */

struct nk_gpio_dev * nk_gpio_dev_find(char *name);

int nk_gpio_dev_get_characteristics(struct nk_gpio_dev *d, struct nk_gpio_dev_characteristics *c);

int nk_gpio_dev_set_output(struct nk_gpio_dev *d, nk_gpio_t, int val);
int nk_gpio_dev_set_input(struct nk_gpio_dev *d, nk_gpio_t);
int nk_gpio_dev_get_type(struct nk_gpio_dev *d, nk_gpio_t, int *type);

int nk_gpio_dev_set_active_low(struct nk_gpio_dev *d, nk_gpio_t);
int nk_gpio_dev_set_active_high(struct nk_gpio_dev *d, nk_gpio_t);
int nk_gpio_dev_get_active_type(struct nk_gpio_dev *d, nk_gpio_t, int *flag);

int nk_gpio_dev_set_open_drain(struct nk_gpio_dev *d, nk_gpio_t);
int nk_gpio_dev_set_open_source(struct nk_gpio_dev *d, nk_gpio_t);
int nk_gpio_dev_get_open_type(struct nk_gpio_dev *d, nk_gpio_t, int *flag);

int nk_gpio_dev_set_pull_up(struct nk_gpio_dev *d, nk_gpio_t);
int nk_gpio_dev_set_pull_down(struct nk_gpio_dev *d, nk_gpio_t);
int nk_gpio_dev_get_pull_type(struct nk_gpio_dev *d, nk_gpio_t, int *flag);

int nk_gpio_dev_write_output(struct nk_gpio_dev *d, nk_gpio_t, int val);
int nk_gpio_dev_read_input(struct nk_gpio_dev *d, nk_gpio_t, int *val);

#endif
