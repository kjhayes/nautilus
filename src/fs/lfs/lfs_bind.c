#include "./lfs.h"
#include "./lfs_util.h"
#include <nautilus/nautilus.h>

// Attach an lfs filesystem to the block device named `devname`
int nk_fs_lfs_attach(char *devname, char *fsname, int readonly) {
  printk("attach %s\n", devname);
  return -1;
}

int nk_fs_lfs_detach(char *fsname) {
  return -1;
}