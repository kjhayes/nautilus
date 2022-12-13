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
 * Copyright (c) 2022, Nick Wanninger
 * Copyright (c) 2022, The V3VEE Project  <http://www.v3vee.org>
 *                     The Hobbes Project <http://xstack.sandia.gov/hobbes>
 * All rights reserved.
 *
 * Authors:  Nick Wanninger <ncw@u.northwestern.edu>
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "LICENSE.txt".
 */

#include "./lfs.h"
#include "./lfs_util.h"

#include <nautilus/blkdev.h>
#include <nautilus/dev.h>
#include <nautilus/fs.h>
#include <nautilus/nautilus.h>

#include <fs/lfs/lfs.h>

#define INFO(fmt, args...) INFO_PRINT("littlefs: " fmt, ##args)
#define DEBUG(fmt, args...) DEBUG_PRINT("littlefs: " fmt, ##args)
#define ERROR(fmt, args...) ERROR_PRINT("littlefs: " fmt, ##args)

#ifndef NAUT_CONFIG_DEBUG_FAT32_FILESYSTEM_DRIVER
#undef DEBUG
#define DEBUG(fmt, args...)
#endif

static void hexdump(const void *data, size_t size) {
  char ascii[17];
  size_t i, j;
  ascii[16] = '\0';
  for (i = 0; i < size; ++i) {
    printk("%02X ", ((unsigned char *)data)[i]);
    if (((unsigned char *)data)[i] >= ' ' &&
        ((unsigned char *)data)[i] <= '~') {
      ascii[i % 16] = ((unsigned char *)data)[i];
    } else {
      ascii[i % 16] = '.';
    }
    if ((i + 1) % 8 == 0 || i + 1 == size) {
      printk(" ");
      if ((i + 1) % 16 == 0) {
        printk("|  %s \n", ascii);
      } else if (i + 1 == size) {
        ascii[(i + 1) % 16] = '\0';
        if ((i + 1) % 16 <= 8) {
          printk(" ");
        }
        for (j = (i + 1) % 16; j < 16; ++j) {
          printk("   ");
        }
        printk("|  %s \n", ascii);
      }
    }
  }
}

struct lfs_state {
  struct nk_block_dev_characteristics chars;
  struct nk_block_dev *dev;
  struct nk_fs *fs;
  // a direct binding to the lfs stuctures
  lfs_t lfs;

  struct lfs_config cfg;
};

// Read a region in a block. Negative error codes are propagated
// to the user.
int lfs_blk_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off,
                 void *buffer, lfs_size_t size) {
  struct lfs_state *s = (struct lfs_state *)c->context;

  INFO("read block %d, off:%zu, size:%zu\n", block, off, size);

  // memset(buffer, 0, size);
  int err = nk_block_dev_read(s->dev, block, 1, buffer, NK_DEV_REQ_BLOCKING,
                              NULL, NULL);
  return err;
}

// Program a region in a block. The block must have previously
// been erased. Negative error codes are propagated to the user.
// May return LFS_ERR_CORRUPT if the block should be considered bad.
int lfs_blk_prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off,
                 const void *buffer, lfs_size_t size) {
  struct lfs_state *s = (struct lfs_state *)c->context;
  INFO("prog block %d, off:%zu\n", block, off);
  int err = nk_block_dev_write(s->dev, block, 1, (void *)buffer,
                               NK_DEV_REQ_BLOCKING, NULL, NULL);
  return err;
}

// Erase a block. A block must be erased before being programmed.
// The state of an erased block is undefined. Negative error codes
// are propagated to the user.
// May return LFS_ERR_CORRUPT if the block should be considered bad.
int lfs_blk_erase(const struct lfs_config *c, lfs_block_t block) {
  struct lfs_state *s = (struct lfs_state *)c->context;

  INFO("erase block %d\n", block);
  // // HACK:
  uint8_t zero[512];
  memset(zero, 0, 512);
  int err = nk_block_dev_write(s->dev, block, 1, zero, NK_DEV_REQ_BLOCKING,
                               NULL, NULL);
  // write zero to a block
  return 0;
}

// Sync the state of the underlying block device. Negative error codes
// are propagated to the user.
int lfs_blk_sync(const struct lfs_config *c) {
  INFO("sync\n");
  // DONT CARE:
  return 0;
}

// Attach an lfs filesystem to the block device named `devname`
int nk_fs_lfs_attach(char *devname, char *fsname, int readonly) {
  struct nk_block_dev *dev = nk_block_dev_find(devname);
  uint64_t flags = readonly ? NK_FS_READONLY : 0;

  if (!dev) {
    ERROR("Cannot find device %s\n", devname);
    return -1;
  }

  struct lfs_state *s = malloc(sizeof(*s));
  if (!s) {
    ERROR("Cannot allocate space for fs %s\n", fsname);
    return -1;
  }

  if (nk_block_dev_get_characteristics(dev, &s->chars) != 0) {
    ERROR("Failed to get block characteristics for device %s\n", fsname);
    free(s);
    return -1;
  }

  INFO("Mount to disk with %d %d byte sectors\n", s->chars.num_blocks,
       s->chars.block_size);

  s->dev = dev;

  memset(&s->cfg, 0, sizeof(struct lfs_config));

  // block device configuration
  // setup the functions:
  s->cfg.read = lfs_blk_read;
  s->cfg.prog = lfs_blk_prog;
  s->cfg.erase = lfs_blk_erase;
  s->cfg.sync = lfs_blk_sync;
  // setup the access size parameters
  s->cfg.block_size = s->chars.block_size;
  s->cfg.read_size = s->chars.block_size;
  s->cfg.prog_size = s->chars.block_size;
  s->cfg.block_count = s->chars.num_blocks;

  s->cfg.cache_size = s->chars.block_size;
  s->cfg.lookahead_size = 8;
  s->cfg.block_cycles = -1;   // no wear leveling
  s->cfg.context = (void *)s; // pass the state to handlerss

  // if (lfs_format(&s->lfs, &s->cfg)) {
  //   ERROR("could not format %s\n", fsname);
  //   free(s);
  //   return -1;
  // }

  if (lfs_mount(&s->lfs, &s->cfg) != 0) {
    ERROR("could not mount %s\n", fsname);
    free(s);
    return -1;
  }

  INFO("Mounted %s\n", fsname);

  lfs_file_t file;
  INFO("trying to open\n");
  int r = lfs_file_open(&s->lfs, &file, "test.txt", LFS_O_RDWR | LFS_O_CREAT);
  INFO("r = %d\n", r);

  char content[512];
  int sz = lfs_file_read(&s->lfs, &file, content, 512);
  INFO("read %d bytes:\n", sz);

  hexdump(content, sz);

  lfs_file_close(&s->lfs, &file);
  lfs_unmount(&s->lfs);

  while (1) {
  }
  return -1;
}

int nk_fs_lfs_detach(char *fsname) { return -1; }