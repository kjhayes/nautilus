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
 * Copyright (c) 2016, Kevin Hayes <kjhayes@u.northwestern.edu>
 * Copyright (c) 2015, The V3VEE Project  <http://www.v3vee.org> 
 *                     The Hobbes Project <http://xstack.sandia.gov/hobbes>
 * All rights reserved.
 *
 * Author: Kevin Hayes <kjhayes@u.northwestern.edu>
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "LICENSE.txt".
 */

#ifndef __NAUTILUS_DEV_INFO_H__
#define __NAUTILUS_DEV_INFO_H__

#include <nautilus/dev.h>
#include <nautilus/errno.h>

/*
 * Device Info
 *
 * Abstraction for the Device tree and ACPI tables based on Linux's fwnode structures
 */

struct nk_dev_info_int {
  const char*(*get_name)(void *state);
  
  int(*has_property)(void *state, const char *prop_name);
  int(*read_int_array)(void *state, const char *prop_name, int elem_size, void *buf, int *buf_cnt);
  int(*read_string_array)(void *state, const char *prop_name, const char **buf, int *buf_cnt);

  int(*read_register_blocks)(void *state, void **bases, size_t *sizes, int *count);
  int(*read_irq)(void *state, int index, nk_irq_t *irq);
  struct nk_dev_info*(*read_irq_parent)(void *state, int index);
  int(*num_irq)(void *state, size_t *num);

  struct nk_dev_info *(*get_parent)(void *state);
  struct nk_dev_info *(*get_child_named)(void *state, const char *child_name);
  struct nk_dev_info *(*children_start)(void *state);
  struct nk_dev_info *(*children_next)(void *state, const struct nk_dev_info *iter);
};

typedef enum nk_dev_info_type {

  NK_DEV_INFO_OF,
  NK_DEV_INFO_ACPI

} nk_dev_info_type_t;

#define NK_DEV_INFO_FLAG_HAS_DEVICE (1<<0)

struct nk_dev_info {

  struct nk_dev *dev;
  struct nk_dev_info_int *interface;

  void *state;

  nk_dev_info_type_t type;
  int flags;
};

inline static int nk_dev_info_set_device(struct nk_dev_info *info, struct nk_dev *device) 
{
  if(info) {
    info->dev = device;
    info->flags |= NK_DEV_INFO_FLAG_HAS_DEVICE;
    return 0;
  } else {
    return -EUNIMPL;
  }
}

inline static struct nk_dev * nk_dev_info_get_device(struct nk_dev_info *info) 
{
  if(info && info->dev && (info->flags & NK_DEV_INFO_FLAG_HAS_DEVICE)) {
    return info->dev;
  } else {
    return NULL;
  }
}

inline static const char * nk_dev_info_get_name(const struct nk_dev_info *info)
{
  if(info && info->interface && info->interface->get_name) {
    return info->interface->get_name(info->state);
  } else {
    return NULL;
  }
}

inline static int nk_dev_info_has_property(const struct nk_dev_info *info, const char *prop_name) 
{
  if(info && info->interface && info->interface->has_property) {
    return info->interface->has_property(info->state, prop_name);
  } else {
    return -EUNIMPL;
  }
}

inline static int nk_dev_info_read_int_array(const struct nk_dev_info *info, const char *prop_name, int elem_size, void *buf, int *cnt) 
{
  if(info && info->interface && info->interface->read_int_array) {
    return info->interface->read_int_array(info->state, prop_name, elem_size, buf, cnt);
  } else {
    return -EUNIMPL;
  }
}
inline static int nk_dev_info_read_int(const struct nk_dev_info *info, const char *prop_name, int size, void *val) 
{
  int read = 1;
  int ret = nk_dev_info_read_int_array(info->state, prop_name, size, val, &read); 
  if(read != 1) {
    ret = -EUNIMPL;
  }
  return ret;
}

#define declare_read_int(BITS) \
  _Static_assert(BITS%8 == 0, "nk_dev_info: declare_read_int needs BITS to be a multiple of 8"); \
  inline static int nk_dev_info_read_u ## BITS (const struct nk_dev_info *info, const char *propname, uint ## BITS ## _t *val) { \
    return nk_dev_info_read_int(info, propname, BITS/8, val); \
  }
#define declare_read_int_array(BITS) \
  _Static_assert(BITS%8 == 0, "nk_dev_info: declare_read_int_array needs BITS to be a multiple of 8"); \
  inline static int nk_dev_info_read_u ## BITS ## _array(const struct nk_dev_info *info, const char *propname, uint ## BITS ## _t *buf, int *cnt) { \
    return nk_dev_info_read_int_array(info, propname, BITS/8, buf, cnt); \
  }

declare_read_int(8) //nk_dev_info_read_u8(...);
declare_read_int_array(8) //nk_dev_info_read_u8_array(...);

declare_read_int(16) //nk_dev_info_read_u16(...);
declare_read_int_array(16) //nk_dev_info_read_u16_array(...);

declare_read_int(32) //nk_dev_info_read_u32(...);
declare_read_int_array(32) //nk_dev_info_read_u32_array(...);

declare_read_int(64) //nk_dev_info_read_u64(...);
declare_read_int_array(64) //nk_dev_info_read_u64_array(...);

inline static int nk_dev_info_read_string_array(const struct nk_dev_info *info, const char *prop_name, const char **buf, int *cnt) 
{
  if(info && info->interface && info->interface->read_string_array) {
    return info->interface->read_string_array(info->state, prop_name, buf, cnt);
  } else {
    return -EUNIMPL;
  }
}
inline static int nk_dev_info_read_string(const struct nk_dev_info *info, const char *prop_name, const char **str)
{
  int read = 1;
  int ret = nk_dev_info_read_string_array(info->state, prop_name, str, &read);
  if(read != 1) {
    ret = -EUNIMPL;
  }
  return ret;
}

inline static int nk_dev_info_read_irq(const struct nk_dev_info *info, int index, nk_irq_t *out_irq) 
{
  if(info && info->interface && info->interface->read_irq) {
    return info->interface->read_irq(info->state, index, out_irq);
  } else {
    return -EUNIMPL;
  }
}

inline static struct nk_dev_info * nk_dev_info_read_irq_parent(const struct nk_dev_info *info, int index) 
{
  if(info && info->interface && info->interface->read_irq_parent) {
    return info->interface->read_irq_parent(info->state, index);
  } else {
    return NULL;
  }
}

inline static int nk_dev_info_num_irq(const struct nk_dev_info *info, size_t *out_num) 
{
  if(info && info->interface && info->interface->num_irq) {
    return info->interface->num_irq(info->state, out_num);
  } else {
    return -EUNIMPL;
  }
}

inline static int nk_dev_info_read_register_blocks(const struct nk_dev_info *info, void** bases, size_t *sizes, int *block_count) 
{
  if(info && info->interface && info->interface->read_register_blocks) {
    return info->interface->read_register_blocks(info->state, bases, sizes, block_count);
  } else {
    return -EUNIMPL;
  }
}
inline static int nk_dev_info_read_register_blocks_exact(const struct nk_dev_info *info, void** bases, size_t *sizes, int block_count) 
{
  int mod_count = block_count;
  int ret = nk_dev_info_read_register_blocks(info, bases, sizes, &mod_count);
  if(mod_count != block_count) {
    ret = -EUNIMPL;
  }
  return ret;
}
inline static int nk_dev_info_read_register_block(const struct nk_dev_info *info, void **base, size_t *size) {
  return nk_dev_info_read_register_blocks_exact(info, base, size, 1);
}

inline static struct nk_dev_info *nk_dev_info_get_parent(const struct nk_dev_info *info) 
{
  if(info && info->interface && info->interface->get_parent) {
    return info->interface->get_parent(info->state);
  } else {
    return NULL;
  }
}

inline static struct nk_dev_info *nk_dev_info_children_start(const struct nk_dev_info *info) 
{
  if(info && info->interface && info->interface->children_start) {
    return info->interface->children_start(info->state);
  } else {
    return NULL;
  }
}
inline static struct nk_dev_info *nk_dev_info_children_next(const struct nk_dev_info *info, const struct nk_dev_info *iter) 
{
  if(info && info->interface && info->interface->children_next) {
    return info->interface->children_next(info->state, iter);
  } else {
    return NULL;
  }
}

#endif
