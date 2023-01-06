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
 * Copyright (c) 2023, Nick Wanninger <ncw@u.northwestern.edu>
 * Copyright (c) 2015, The V3VEE Project  <http://www.v3vee.org>
 *                     The Hobbes Project <http://xstack.sandia.gov/hobbes>
 * All rights reserved.
 *
 * Author: Nick Wanninger <ncw@u.northwestern.edu>
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "LICENSE.txt".
 */
#include <nautilus/nautilus.h>
#include <nautilus/shell.h>
#include <nautilus/user.h>

#define SEG_NULL 1  // kernel code
#define SEG_KCODE 1 // kernel code
#define SEG_KDATA 2 // kernel data+stack
#define SEG_KCPU 3  // kernel per-cpu data
#define SEG_UCODE 4 // user code
#define SEG_UDATA 5 // user data+stack
#define SEG_TSS 6   // this process's task state

static inline void user_lgdt(void *data, int size) {
  struct gdt_desc64 gdt;
  gdt.limit = size - 1;
  gdt.base = (uint64_t)data;

  asm volatile("lgdt %0" ::"m"(gdt));
}
static inline void user_ltr(uint16_t sel) {
  asm volatile("ltr %0" : : "r"(sel));
}

void nk_user_init(void) {
  printk("initializing userspace\n");
  // Load a new gdt which includes the userspace flags
  struct cpu *cpu = get_cpu();
  cpu->tss = kmem_malloc(4096);

  uint32_t *tss = (uint32_t *)cpu->tss;
  uint64_t *gdt = (uint64_t *)(((char *)tss) + 1024);

  tss[16] = 0x00680000; // IO Map Base = End of TSS
  tss[0x64] |= (0x64 * sizeof(uint32_t)) << 16;

  // Load a fresh GDT to enable userspace isolation. Loading a new one now we
  // are in C is easier than if we hardcoded all this in the boot.S file
  // considering the TSS needs to be configured correctly so we can have a
  // kernel stack
  uint64_t addr = (uint64_t)tss;
  gdt[0] = 0x0000000000000000;
  gdt[SEG_KCODE] = 0x002098000000FFFF; // Code, DPL=0, R/X
  gdt[SEG_KDATA] = 0x000092000000FFFF; // Data, DPL=0, W
  gdt[SEG_KCPU] = 0x000000000000FFFF;  // unused
  gdt[SEG_UCODE] = 0x0020F8000000FFFF; // Code, DPL=3, R/X
  gdt[SEG_UDATA] = 0x0000F2000000FFFF; // Data, DPL=3, W
  gdt[SEG_TSS + 0] = (0x0067) | ((addr & 0xFFFFFF) << 16) | (0x00E9LL << 40) |
                     (((addr >> 24) & 0xFF) << 56);
  gdt[SEG_TSS + 1] = (addr >> 32);

  printk("Loading new userspace-capabale gdt\n");
  user_lgdt((void *)gdt, 8 * sizeof(uint64_t));
  user_ltr(SEG_TSS << 3);

  printk("Userspace enabled on core %d\n", cpu->id);
}

static int handle_urun(char *buf, void *priv) {
  nk_vc_printf("hello\n");
  //
  return 0;
}

static struct shell_cmd_impl urun_impl = {
    .cmd = "urun",
    .help_str = "urun path <arg>",
    .handler = handle_urun,
};
nk_register_shell_cmd(urun_impl);