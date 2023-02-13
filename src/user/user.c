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

#define ERROR(fmt, args...) ERROR_PRINT("userspace: " fmt, ##args)

static inline void user_lgdt(void *data, int size) {
  struct gdt_desc64 gdt;
  gdt.limit = size - 1;
  gdt.base = (uint64_t)data;

  asm volatile("lgdt %0" ::"m"(gdt));
}
static inline void user_ltr(uint16_t sel) {
  asm volatile("ltr %0" : : "r"(sel));
}

// Called per CPU, this function initializes the CPU state in a way that enables
// it to enter ring 3 code. THe main thing that it does is configures a new GDT
// with segments for user code and data, as well as a segment for the TSS
// (task state segment) which is where the IST lives
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
  gdt[SEG_KCODE] = 0x00af9a000000ffff; // Code, DPL=0, R/X
  gdt[SEG_KDATA] = 0x00af92000000ffff; // Data, DPL=0, W
  gdt[SEG_KCPU] = 0x000000000000FFFF;  // unused
  gdt[SEG_UCODE] = 0x0020F8000000FFFF; // Code, DPL=3, R/X
  gdt[SEG_UDATA] = 0x0000F2000000FFFF; // Data, DPL=3, W
  gdt[SEG_TSS + 0] = (0x0067) | ((addr & 0xFFFFFF) << 16) | (0x00E9LL << 40) |
                     (((addr >> 24) & 0xFF) << 56);
  gdt[SEG_TSS + 1] = (addr >> 32);

  printk("Loading new userspace-capabale gdt\n");
  user_lgdt((void *)gdt, 8 * sizeof(uint64_t));
  user_ltr(SEG_TSS << 3);

  // msr_write(0xC0000102, msr_read(0xC0000101));
  printk("Userspace enabled on core %d\n", cpu->id);
}

nk_thread_t *user_get_thread() { return get_cur_thread(); }

// Low level interrupt handler for system call requests
int user_syscall_handler(excp_entry_t *excp, excp_vec_t vector, void *state) {
  struct user_frame *r = (struct user_frame *)((addr_t)excp + -128);
  per_cpu_get(system);
  nk_process_t *proc = get_cur_process();
  if (proc == NULL) {
    return -1; // TODO: what?
  }

  // uint64_t sp;
  // asm( "mov %%rsp, %0" : "=rm" ( sp ));
  // printk("in  %d sp before %p. rax=%d\n", get_cur_thread()->tid, sp, r->rax);
  r->rax = process_dispatch_syscall(proc, r->rax, r->rdi, r->rsi, r->rdx);
  // asm( "mov %%rsp, %0" : "=rm" ( sp ));
  // printk("out %d sp before %p\n", get_cur_thread()->tid, sp);
  
  return 0;
}


void mr_burns_task(void *arg, void **out) {
  while (1) {
    // nk_sched_yield(NULL);
  }
}
static int handle_urun(char *buf, void *priv) {

  // nk_thread_id_t mr_burns;
  // nk_thread_create(mr_burns_task, NULL, NULL, 0, 4096, &mr_burns, CPU_ANY);
  // nk_thread_name(mr_burns, "mr_burns");
  // nk_thread_run(mr_burns);

  // for (int i = 0; i < 10; i++) {
  //   nk_process_t *proc = nk_process_create("hello", "");
  //   if (proc == NULL) {
  //     ERROR("Could not spawn process.\n");
  //     return 0;
  //   }
  //   // wait for the process (it's main thread) to exit, and free it's memory
  //   nk_process_wait(proc);
  // }

  // return 0;




  // return 0;
  // TODO: parse the command
  char command[256];
  char argument[256];
  int scanned = sscanf(buf, "urun %s %s", command, argument);
  // no argument passed
  if (scanned < 1) {
    strcpy(command, "/init");
  }
  if (scanned < 2) {
    strcpy(argument, "");
  }

  // create the process
  nk_process_t *proc = nk_process_create(command, argument);
  if (proc == NULL) {
    ERROR("Could not spawn process.\n");
    return 0;
  }

  // wait for the process (it's main thread) to exit, and free it's memory
  nk_process_wait(proc);

  return 0;
}

static struct shell_cmd_impl urun_impl = {
    .cmd = "urun",
    .help_str = "urun <path> <arg>",
    .handler = handle_urun,
};
nk_register_shell_cmd(urun_impl);
