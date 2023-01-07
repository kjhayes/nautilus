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

struct user_frame {
  uint64_t r15;
  uint64_t r14;
  uint64_t r13;
  uint64_t r12;
  uint64_t r11;
  uint64_t r10;
  uint64_t r9;
  uint64_t r8;
  uint64_t rbp;
  uint64_t rdi;
  uint64_t rsi;
  uint64_t rdx;
  uint64_t rcx;
  uint64_t rbx;
  uint64_t rax;

  uint64_t trapno;
  uint64_t err;

  uint64_t rip;
  uint64_t cs;
  uint64_t rflags;
  uint64_t rsp;
  uint64_t ds;
} __packed;

/**
 * This file is the main implementation of the nautilus kernel simple userspace
 * system.
 */

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

static void tss_set_rsp(uint32_t *tss, uint32_t n, uint64_t rsp) {
  tss[n * 2 + 1] = rsp;
  tss[n * 2 + 2] = rsp >> 32;
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

extern void __attribute__((noreturn)) user_start(void *tf_rsp, int data_segment);

#define syscall0(nr)                                                           \
  ({                                                                           \
    unsigned long ret;                                                         \
    asm volatile("int $0x80" : "=a"(ret) : "0"(nr) : "memory");                \
    ret;                                                                       \
  });


static volatile int val;
void user_code(void) {
	val = 0;
  syscall0(1);
  while (1) {
		//
  }
}


nk_thread_t *user_get_thread() { return get_cur_thread(); }

int user_syscall_handler(excp_entry_t *excp, excp_vec_t vector, void *state) {
  // nk_vc_printf("in the syscall handler\n");
  struct user_frame *r = (struct user_frame *)((addr_t)excp + -128);

	per_cpu_get(system);

	r->rax = read_cr3();
  // r->rax = 42;
  return 0;
}


static void thread_run_user(void *input, void **output) {
  nk_thread_t *t = get_cur_thread();

  struct user_frame frame;
  frame.rip = (uint64_t)user_code;     // userspace code
  frame.cs = (SEG_UCODE << 3) | 3;     // user code segment in ring 3
  frame.rflags = 0x00000200;           // enable interrupts
  frame.rsp = (uint64_t)t->stack + 32; // userspace stack
  frame.ds = (SEG_UDATA << 3) | 3;     // user data segment in ring 3

  tss_set_rsp(get_cpu()->tss, 0, (uint64_t)t->stack + t->stack_size);
  user_start((void *)&frame, SEG_UDATA);

  nk_thread_exit(NULL);
}

static int handle_urun(char *buf, void *priv) {
	
	per_cpu_get(system);

  // spawn a new thread that will be used as a userspace thread
  nk_thread_id_t tid = 0;
  if (nk_thread_create(thread_run_user, buf, NULL, 0, 4096, &tid, CPU_ANY) <
      0) {
    return -1;
  }

  // set the thread's name for debugging
  nk_thread_name(tid, buf);

  nk_thread_t *t = tid;
  t->vc = get_cur_thread()->vc;

  // Start the thread
  nk_thread_run(tid);
  // Wait for it to finish
  nk_join(tid, NULL);
  return 0;
}

static struct shell_cmd_impl urun_impl = {
    .cmd = "urun",
    .help_str = "urun path <arg>",
    .handler = handle_urun,
};
nk_register_shell_cmd(urun_impl);
