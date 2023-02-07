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
#define LEN_4GB (0x100000000UL)

static spinlock_t ptable_lock;
// A simple (slow) linked list that represents the "process table"
static struct list_head ptable_list;

static int proc_next_pid = 1; // start with pid 1
#define PTABLE_LOCK_CONF uint8_t _ptable_lock_flags
#define PTABLE_LOCK() _ptable_lock_flags = spin_lock_irq_save(&ptable_lock)
#define PTABLE_UNLOCK()                                                        \
  spin_unlock_irq_restore(&ptable_lock, _ptable_lock_flags);

#define syscall0(nr)                                                           \
  ({                                                                           \
    unsigned long ret;                                                         \
    asm volatile("int $0x80" : "=a"(ret) : "0"(nr) : "memory");                \
    ret;                                                                       \
  });

extern void __attribute__((noreturn))
user_start(void *tf_rsp, int data_segment);
static volatile int val;
void user_code(void) {
  val = 0;
  syscall0(1);
  while (1) {
    //
  }
}

static void tss_set_rsp(uint32_t *tss, uint32_t n, uint64_t rsp) {
  tss[n * 2 + 1] = rsp;
  tss[n * 2 + 2] = rsp >> 32;
}

static nk_aspace_interface_t nk_process_aspace_interface = {};

static void process_run_user(void *input, void **output) {
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



/*
 * Setup a process' address space. The main thing that happens here is we
 * create a paging aspace, attach it to the process and the main thread, then
 * add a region for the kernel to be mapped into, then we create a region
 * for the stack and the user binary.
 *
 * Returns: 0 on success, -1 on failure.
 *     The aspace and all intermediate structs will be cleaned up on failure
 */
static int setup_process_aspace(nk_process_t *proc, nk_aspace_characteristics_t *aspace_chars) {

  nk_aspace_region_t region;
  nk_thread_t *thread = NULL;
  char aspace_name[32];


  // allocate an aspace for the process
  sprintf(aspace_name, "pid%d\n", proc->pid);
  proc->aspace = nk_aspace_create("paging", aspace_name, &aspace_chars);
  if (proc->aspace == NULL) {
    ERROR("Failed to allocate aspace for process!\n");
    goto clean_up;
  }
  thread = proc->main_thread;
  thread->aspace = proc->aspace;

  // create a 1-1 region mapping all of physical memory so that
  // the kernel can work when the process is active
  region.va_start = 0;
  region.pa_start = 0;
  region.len_bytes = LEN_4GB;  // first 4 GB are mapped
  // set protections for kernel
  // use EAGER to tell paging implementation that it needs to build all these PTs right now
  region.protect.flags = NK_ASPACE_READ | NK_ASPACE_WRITE | NK_ASPACE_EXEC | NK_ASPACE_PIN | NK_ASPACE_KERN | NK_ASPACE_EAGER;

  // Add the region to the aspace, which should map the memory immediately.
  if (nk_aspace_add_region(proc->aspace, &region)) {
    ERROR("failed to add initial eager region to address space\n");
    goto clean_up;
  }
  return 0;

clean_up:
  // Delete the aspace if we ended up creating it.
  if (proc->aspace != NULL) nk_aspace_destroy(proc->aspace);
  return -1;
}

nk_process_t *nk_process_create(const char *program, const char *argument) {
  PTABLE_LOCK_CONF;

  nk_process_t *proc = NULL;
  nk_thread_t *thread;

  // First, we query the aspace implementation to make
  // sure it has a paging implmentation for us.
  nk_aspace_characteristics_t aspace_chars;
  if (nk_aspace_query("paging", &aspace_chars) != 0) {
    ERROR("No paging implementation available\n");
    // TODO error
    return NULL;
  }


  // allocate the memory for the process
  proc = malloc(sizeof(*proc));
  if (!proc) {
    ERROR("cannot allocate process\n");
    goto clean_up;
  }
  // Make sure the process structure is zeroed
  memset(proc, 0, sizeof(*proc));

  // spawn the first thread
  if (nk_thread_create(process_run_user, (void *)argument, NULL, 0, 4096,
                       &proc->main_thread, CPU_ANY) < 0) {
    ERROR("cannot allocate main thread for process\n");
    goto clean_up;
  }

  // now that we have allocated the process and it's first thread, we should be good to go!
  // First, we'll configure the process' name and update the main thread's name to match.
  strncpy(proc->program, program, sizeof(proc->program) - 1);
  nk_thread_name(&proc->main_thread, proc->program);

  // Allocate a pid and insert this process into the process table.
  PTABLE_LOCK();
  // allocate a pid
  proc->pid = proc_next_pid++;
  list_add_tail(&proc->ptable_list_node, &ptable_list);
  PTABLE_UNLOCK()

  if (setup_process_aspace(proc, &aspace_chars)) {
    ERROR("Failed to initialize process' address space\n");
    goto clean_up;
  }

  thread = proc->main_thread;
  thread->vc = get_cur_thread()->vc; // Make sure the thread prints to the console


  // kickoff main thread
  nk_thread_run(proc->main_thread);

  nk_vc_printf("Here: pid=%d\n", proc->pid);
  return proc;


clean_up:
  if (proc != NULL) {
    // Delete the main thread
    if (proc->main_thread != NULL) nk_thread_destroy(proc->main_thread);
    // Finally, delete the process
    PTABLE_LOCK(); // First, remove it from the table list
    list_del(&proc->ptable_list_node);
    PTABLE_UNLOCK();
    // ...

    // Finally, delete the process
    free(proc);
  }
  return NULL;
}

// mark the process as exited, and exit the main thread
// (this MUST be called from the main thread)
void nk_process_exit(nk_process_t *process) {}

int nk_process_wait(nk_process_t *process) {

  nk_join(process->main_thread, NULL);
	return 0;
}
