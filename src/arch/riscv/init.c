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
 * Copyright (c) 2015, Kyle C. Hale <kh@u.northwestern.edu>
 * Copyright (c) 2015, The V3VEE Project  <http://www.v3vee.org>
 *                     The Hobbes Project <http://xstack.sandia.gov/hobbes>
 * All rights reserved.
 *
 * Authors: Kyle C. Hale <kh@u.northwestern.edu>
 *          Peter Dinda <pdinda@northwestern.edu> (Gem5 variant)
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "LICENSE.txt".
 */
#define __NAUTILUS_MAIN__

#include <nautilus/nautilus.h>
#include <nautilus/init.h>
#include <nautilus/arch.h>
#include <nautilus/paging.h>
#include <nautilus/barrier.h>
#include <nautilus/blkdev.h>
#include <nautilus/chardev.h>
#include <nautilus/irqdev.h>
#include <nautilus/gpiodev.h>
#include <nautilus/gpio.h>
#include <nautilus/endian.h>
#include <nautilus/cmdline.h>
#include <nautilus/cpu.h>
#include <nautilus/dev.h>
#include <nautilus/errno.h>
#include <nautilus/fs.h>
#include <nautilus/future.h>
#include <nautilus/gpudev.h>
#include <nautilus/group.h>
#include <nautilus/group_sched.h>
#include <nautilus/idle.h>
#include <nautilus/interrupt.h>
#include <nautilus/libccompat.h>
#include <nautilus/linker.h>
#include <nautilus/mb_utils.h>
#include <nautilus/mm.h>
#include <nautilus/msg_queue.h>
#include <nautilus/netdev.h>
#include <nautilus/percpu.h>
#include <nautilus/prog.h>
#include <nautilus/random.h>
#include <nautilus/semaphore.h>
#include <nautilus/shell.h>
#include <nautilus/smp.h>
#include <nautilus/spinlock.h>
#include <nautilus/task.h>
#include <nautilus/thread.h>
#include <nautilus/timer.h>
#include <nautilus/vc.h>
#include <nautilus/waitqueue.h>
#include <nautilus/timehook.h>

#include <nautilus/of/dt.h>
#include <nautilus/of/fdt.h>

#ifdef NAUT_CONFIG_ENABLE_REMOTE_DEBUGGING
#include <nautilus/gdb-stub.h>
#endif

#ifdef NAUT_CONFIG_SIFIVE_PLIC
#include <dev/sifive_plic.h>
#endif

#ifdef NAUT_CONFIG_SIFIVE_GPIO
#include <dev/sifive_gpio.h>
#endif

#include <arch/riscv/sbi.h>
#include <arch/riscv/trap.h>
#include <arch/riscv/riscv_idt.h>
#include <arch/riscv/npb.h>
#include <arch/riscv/hlic.h>

#define QUANTUM_IN_NS (1000000000ULL / NAUT_CONFIG_HZ)

struct nk_sched_config sched_cfg = {
    .util_limit = NAUT_CONFIG_UTILIZATION_LIMIT * 10000ULL,  // convert percent to 10^-6 units
    .sporadic_reservation = NAUT_CONFIG_SPORADIC_RESERVATION * 10000ULL,  // ..
    .aperiodic_reservation = NAUT_CONFIG_APERIODIC_RESERVATION * 10000ULL,  // ..
    .aperiodic_quantum = QUANTUM_IN_NS,
    .aperiodic_default_priority = QUANTUM_IN_NS,
};

static int sysinfo_init(struct sys_info *sys) {
  sys->core_barrier = (nk_barrier_t *)malloc(sizeof(nk_barrier_t));
  if (!sys->core_barrier) {
    ERROR_PRINT("Could not allocate core barrier\n");
    return -1;
  }
  memset(sys->core_barrier, 0, sizeof(nk_barrier_t));

  if (nk_barrier_init(sys->core_barrier, sys->num_cpus) != 0) {
    ERROR_PRINT("Could not create core barrier\n");
    goto out_err;
  }

  return 0;

out_err:
  free(sys->core_barrier);
  return -EINVAL;
}

#define NAUT_WELCOME                                      \
  "Welcome to                                         \n" \
  "    _   __               __   _  __                \n" \
  "   / | / /____ _ __  __ / /_ (_)/ /__  __ _____    \n" \
  "  /  |/ // __ `// / / // __// // // / / // ___/    \n" \
  " / /|  // /_/ // /_/ // /_ / // // /_/ /(__  )     \n" \
  "/_/ |_/ \\__,_/ \\__,_/ \\__//_//_/ \\__,_//____/  \n" \
  "+===============================================+  \n" \
  " Kyle C. Hale (c) 2014 | Northwestern University   \n" \
  "+===============================================+  \n\n"

extern uint8_t  init_smp_boot[];
extern void * secondary_core_stack;
extern uint64_t _bssStart[];
extern uint64_t _bssEnd[];

// KJH - The stack switch which happens halfway through "init" makes this needed (I think)
static const char *chardev_name = NAUT_CONFIG_VIRTUAL_CONSOLE_CHARDEV_CONSOLE_NAME;

extern struct naut_info *smp_ap_stack_switch(uint64_t, uint64_t,
                         struct naut_info *);

bool_t second_done = false;

void * secondary_entry_boot_stack(int hartid) 
{
  w_tp(0);

  printk("RISCV: hart %d started!\n", hartid);

  struct naut_info *naut = &nautilus_info;
  cpu_id_t cpuid = riscv_hartid_to_cpu_id(hartid);

  write_csr(sscratch, r_tp());

  w_tp((uint64_t)naut->sys.cpus[cpuid]);

  // Write supervisor trap vector location
  trap_init();

  if(smp_xcall_init_queue(naut->sys.cpus[cpuid])) {
    WARN_PRINT("Failed to initialize xcall queue on hart %u!\n", hartid);
  }

  if(hlic_percpu_init()) {
    panic("Failed to initialize the HLIC locally for CPU %u!\n", my_cpu_id());
  }

#ifdef NAUT_CONFIG_SIFIVE_PLIC
  /* Initialize the platform level interrupt controller for this HART */
  if(plic_percpu_init()) {
    panic("Failed to initialize the PLIC locally for CPU %u!\n", my_cpu_id()); 
  }
#endif

  nk_rand_init(naut->sys.cpus[cpuid]);

  nk_sched_init_ap(&sched_cfg);

  /* set the timer with sbi :) */
  // sbi_legacy_set_timer(rv::get_time() + TICK_INTERVAL);
  sbi_legacy_set_timer(read_csr(time) + TICK_INTERVAL);

  // We need to stop using our stack at this point because 
  // it might be being used by another core or freed after
  second_done = true;

  return get_cur_thread()->rsp;
}

void secondary_entry_threaded(void) {
  
  uint32_t hartid = per_cpu_get(hartid);

  printk("hart %u swapped stacks\n", hartid);

  nk_sched_start();

  printk("started scheduling on hart %u\n", hartid);

  arch_enable_ints();

  printk("hart %u idling...\n", hartid);

  idle(NULL, NULL);
}

int start_secondary(struct sys_info *sys) {

  secondary_core_stack = (uint64_t)malloc(PAGE_SIZE_2MB);
  secondary_core_stack += PAGE_SIZE_2MB;

  for (int i = 0; i < NAUT_CONFIG_MAX_CPUS; i++) {
    if (i == my_cpu_id() || !sys->cpus[i] || !sys->cpus[i]->enabled) continue;

    second_done = false;
    __sync_synchronize();

    struct sbiret ret = sbi_call(SBI_EXT_HSM, SBI_EXT_HSM_HART_START, i, &init_smp_boot, 1);
    if (ret.error != SBI_SUCCESS) {
      continue;
    }

    printk("RISCV: hart %d is trying to start hart %d\n", my_cpu_id(), i);

    while (second_done != true) {
      __sync_synchronize();
    }

    printk("RISCV: hart %d successfully started hart %d\n", my_cpu_id(), i);
  }

  // Every core has been started
  free(secondary_core_stack);
  return 0;
}

void my_monitor_entry(void);

__attribute__((noinline)) int do_some_work(int x) {
  for (int i = 0; i < 10000; i++) {
    x = x % i;
  }
  return x;
}

#ifdef NAUT_CONFIG_BEANDIP
__attribute__((annotate("nohook"))) 
#endif
void * boot_stack_init(unsigned long hartid, unsigned long fdt) {

  if (!fdt) panic("Invalid FDT: %p\n", fdt);

  nk_low_level_memset(_bssStart, 0, (off_t)_bssEnd - (off_t)_bssStart);

  // Write supervisor trap vector location
  trap_init();

  // Get necessary information from SBI
  sbi_early_init();

  // M-Mode passes scratch struct through tp. Move it to sscratch
  write_csr(sscratch, r_tp());

  // Zero out tp for now until cls is set up
  // w_tp(0);

  struct naut_info *naut = &nautilus_info;
  nk_low_level_memset(naut, 0, sizeof(struct naut_info));

  naut->sys.bsp_id = hartid;
  naut->sys.dtb = (struct dtb_fdt_header *)fdt;

  nk_handle_init_stage_silent();

  /*
  printk("RISCV: hart %d mvendorid: %llx\n", hartid, sbi_call(SBI_GET_MVENDORID).value);
  printk("RISCV: hart %d marchid:   %llx\n", hartid, sbi_call(SBI_GET_MARCHID).value);
  printk("RISCV: hart %d mimpid:    %llx\n", hartid, sbi_call(SBI_GET_MIMPID).value);
  */

  print_fdt(fdt);

  // asm volatile ("wfi");

  nk_handle_init_stage_static();

  // Setup the temporary boot-time allocator
  mm_boot_init(fdt);

  nk_handle_init_stage_boot();

  // Enumate CPUs and initialize them
  arch_smp_early_init(naut);

  /* this will populate NUMA-related structures */
  arch_numa_init(&naut->sys);

  /* this will finish up the identity map */
  nk_paging_init(&(naut->sys.mem), fdt);

  // Setup the main kernel memory allocator
  nk_kmem_init();

  // Setup per-core area for BSP
  w_tp((uint64_t)naut->sys.cpus[hartid]);

  /* now we switch to the real kernel memory allocator, pages
   * allocated in the boot mem allocator are kept reserved */
  mm_boot_kmem_init();

  nk_handle_init_stage_kmem();

#ifdef NAUT_CONFIG_SIFIVE_GPIO
  sifive_gpio_init(fdt);
#endif

  // my_monitor_entry();

  //sbi_init();

  sysinfo_init(&(naut->sys));

  nk_gpio_init();

  nk_wait_queue_init();
  nk_future_init();
  nk_timer_init();
  nk_rand_init(naut->sys.cpus[hartid]);
  nk_semaphore_init();
  nk_msg_queue_init();
  nk_sched_init(&sched_cfg);
  nk_thread_group_init();
  nk_group_sched_init();

  /* we now switch away from the boot-time stack */
  return get_cur_thread()->rsp;
}

void threaded_init(void) {

  nk_thread_name(get_cur_thread(), "init");

  printk("Swapped stacks\n");

  struct naut_info *naut = nk_get_nautilus_info();
  unsigned long hartid = per_cpu_get(hartid);

  /* mm_boot_kmem_cleanup(); */

  if(hlic_init()) {
    panic("failed to initialize the HLIC!\n");
  }
  if(hlic_percpu_init()) {
    panic("failed to initialize the HLIC on BSP!\n");
  }

#ifdef NAUT_CONFIG_SIFIVE_PLIC
  if(plic_init()) {
    panic("failed to initialized the PLIC!\n");
  }
  if(plic_percpu_init()) {
    panic("Failed to initialize the PLIC locally for the BSP!\n"); 
  }
#endif

  if(smp_setup_xcall_bsp(naut->sys.cpus[riscv_hartid_to_cpu_id(hartid)])) {
    WARN_PRINT("Failed to setup xcall's on bsp!\n");
  }

  arch_enable_ints();

  /* interrupts are now on */

  start_secondary(&(naut->sys));

  nk_sched_start();

  nk_handle_init_stage_subsys();

  nk_handle_init_stage_sched();

  nk_handle_init_stage_driver();

  nk_handle_init_stage_fs();

  nk_dump_all_irq();
  // // nk_linker_init(naut);
  // // nk_prog_init(naut);

  // // nk_loader_init();

  // // nk_pmc_init();

  //nk_cmdline_init(naut);
  // nk_test_init(naut);

  // // kick off the timer subsystem by setting a timer sometime in the future
  // sbi_set_timer(read_csr(time) + TICK_INTERVAL);

  // // sifive_test();
  // /* my_monitor_entry(); */

#ifdef NAUT_CONFIG_BEANDIP
  nk_time_hook_start();
#endif

  printk("Current CPU: %d\n", my_cpu_id());

  nk_handle_init_stage_launch();

  printk("Promoting init thread to idle\n");

  idle(NULL,NULL);
}

void init_simple(unsigned long hartid, unsigned long fdt) {

  if (!fdt) panic("Invalid FDT: %p\n", fdt);

  /* nk_low_level_memset(_bssStart, 0, (off_t)_bssEnd - (off_t)_bssStart); */

  // Write supervisor trap vector location
  trap_init();

  // Zero out tp for now until cls is set up
  w_tp(0);

  struct naut_info *naut = &nautilus_info;
  nk_low_level_memset(naut, 0, sizeof(struct naut_info));

  // Initialize platform level interrupt controller for this HART

  if(hlic_percpu_init()) {
    panic("Failed to initialize the HLIC locally for CPU %u!\n", my_cpu_id());
  }

#ifdef NAUT_CONFIG_SIFIVE_PLIC
  /* Initialize the platform level interrupt controller for this HART */
  if(plic_percpu_init()) {
    panic("Failed to initialize the PLIC locally for CPU %u!\n", my_cpu_id()); 
  }
#endif

  arch_enable_ints();

  // We now have serial output without SBI
  //sifive_serial_init(fdt);

  printk("hartid: %ld, fdt: %p (%x) \n", hartid, fdt, *(uint32_t*)fdt);

  //plic_dump();

  my_monitor_entry();

  //sifive_test();
}


/* Faking some vc stuff */

uint16_t
vga_make_entry (char c, uint8_t color)
{
    uint16_t c16 = c;
    uint16_t color16 = color;
    return c16 | color16 << 8;
}


/* Some threading stuff for monitor */

static bool_t done = false;

static void print_ones(void)
{
    while (!done) {
        printk("1");
    }
}

static void print_twos(void)
{
    while (!done) {
        printk("2");
    }
}

int execute_threading(char command[])
{
    nk_thread_id_t a, b;
    nk_thread_start((nk_thread_fun_t)print_ones, 0, 0, 0, 0, &a, my_cpu_id());
    nk_thread_start((nk_thread_fun_t)print_twos, 0, 0, 0, 0, &b, my_cpu_id());

    done = false;
    int i = 0;
    while (i < 10) {
        nk_yield();
        i++;
    }
    printk("\n");
    done = true;
    nk_join(a, NULL);
    nk_join(b, NULL);

    return 0;
}
