#!/usr/bin/env bash

set -e

# make the root filesystem (lfs)
make ramdisk.img

# build the kernel and the isoimage for it
make isoimage -j $(nproc)

qemu-system-x86_64 -cdrom nautilus.iso -m 2G -nographic -serial mon:stdio