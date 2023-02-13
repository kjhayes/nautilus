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
#include <nautilus/fs.h>
#include <nautilus/macros.h>
#include <nautilus/nautilus.h>
#include <nautilus/shell.h>
#include <nautilus/user.h>

#define ERROR(fmt, args...) ERROR_PRINT("userspace: " fmt, ##args)
#define LEN_4GB (0x100000000UL)

// ================ PROCESS TABLE STRUCTURES ================
static spinlock_t ptable_lock;
// A simple (slow) linked list that represents the "process table"
static struct list_head ptable_list = LIST_HEAD_INIT(ptable_list);
static int proc_next_pid = 1; // start with pid 1
#define PTABLE_LOCK_CONF uint8_t _ptable_lock_flags
#define PTABLE_LOCK() _ptable_lock_flags = spin_lock_irq_save(&ptable_lock)
#define PTABLE_UNLOCK()                                                        \
  spin_unlock_irq_restore(&ptable_lock, _ptable_lock_flags);

#define PROCESS_LOCK_CONF uint8_t _process_lock_flags
#define PROCESS_LOCK(proc)                                                     \
  _process_lock_flags = spin_lock_irq_save(&proc->process_lock)
#define PROCESS_UNLOCK(proc)                                                   \
  spin_unlock_irq_restore(&proc->process_lock, _process_lock_flags);
// ==========================================================

// The assembly function which calls `iret` to enter userspace from the kernel.
extern void __attribute__((noreturn))
user_start(void *tf_rsp, int data_segment);
// Set the CPU trap stack pointer (Which stack should be used when an interrupt
// happens in userspace?)
static void tss_set_rsp(uint32_t *tss, uint32_t n, uint64_t rsp) {
  tss[n * 2 + 1] = rsp;
  tss[n * 2 + 2] = rsp >> 32;
}

// This function is called whenever a thread is about to be context
// switched into. The main function is to initialize some CPU state
// so that the context switch into userspace can succeed.
void nk_process_switch(nk_thread_t *t) {
  if (t->process == NULL) {
    tss_set_rsp(get_cpu()->tss, 0, 0);
    return; // NOP
  }
  tss_set_rsp(get_cpu()->tss, 0,
              (uint64_t)t->stack + t->stack_size - (1 * sizeof(uint64_t)));
}

// Given a thread, return the process it belongs to
nk_process_t *get_process(nk_thread_t *thread) { return thread->process; }
// Get the current process for the currently executing thread.s
nk_process_t *get_cur_process(void) { return get_process(get_cur_thread()); }
// Lookup a process structure by it's pid. This implementation is really slow,
// as it walks the process table as a linear linked list. A better
// implementation would be a hashmap or a red-black tree.
static nk_process_t *get_process_pid(long pid) {
  PTABLE_LOCK_CONF;
  PTABLE_LOCK();

  // walk the list to get the pid
  struct list_head *cur;
  list_for_each(cur, &ptable_list) {
    nk_process_t *proc = list_entry(cur, nk_process_t, ptable_list_node);
    if (proc->pid == pid) {
      PTABLE_UNLOCK();
      return proc;
    }
  }

  PTABLE_UNLOCK();
  return NULL;
}

static addr_t process_valloc(nk_process_t *proc, off_t incr_pages) {
  PROCESS_LOCK_CONF;

  PROCESS_LOCK(proc);
  off_t bytes = incr_pages * PAGE_SIZE_4KB;
  off_t vaddr = proc->next_palloc;
  proc->next_palloc += bytes + PAGE_SIZE_4KB; // bump the next pointer, skipping
                                              // one page (for "protection")

  PROCESS_UNLOCK(proc);

  // Allocate the region
  nk_aspace_region_t stack_region;
  stack_region.va_start = (void *)vaddr;
  stack_region.pa_start = 0; // anonymous.
  stack_region.len_bytes = bytes;
  // Readable, writable, anonymous, and eager.
  // Note: NOT executable
  stack_region.protect.flags = NK_ASPACE_READ | NK_ASPACE_WRITE |
                               NK_ASPACE_EXEC | NK_ASPACE_ANON |
                               NK_ASPACE_EAGER;
  nk_aspace_add_region(proc->aspace,
                       &stack_region); // cross fingers this works!

  return vaddr;
}

static nk_fs_fd_t process_get_fd(nk_process_t *proc, int fd) {
  if (fd < 0 || fd > PROCESS_FD_TABLE_SIZE) {
    // invalid file decriptor
    return FS_BAD_FD;
  }
  // already closed
  if (proc->open_files[fd] == FS_BAD_FD) {
    return FS_BAD_FD;
  }

  return proc->open_files[fd];
}

// Dispatch on a system call `nr` on process `proc
unsigned long process_dispatch_syscall(nk_process_t *proc, int nr, uint64_t a,
                                       uint64_t b, uint64_t c) {

  // TODO: add more system calls!
  if (nr == SYSCALL_EXIT) {
    nk_thread_exit(NULL);
    return 0;
  }

  // Write some characters to the console
  if (nr == SYSCALL_CONWRITE) {
    // TODO: reading directly from user memory is dangerous.
    //       see /user/bin/hack.c to know why...
    //       How could we fix this?
    char *x = (char *)a;
    for (int i = 0; i < b; i++) {
      nk_vc_printf("%c", x[i]);
    }
    return 0;
  }

  if (nr == SYSCALL_GETC) {
    // probably a bad idea to forward directly... but whatever
    return nk_vc_getchar(a);
  }

  if (nr == SYSCALL_SPAWN) {
    const char *program = (const char *)a;  // TODO: copy_from_user
    const char *argument = (const char *)b; // TODO: copy_from_user
    nk_process_t *proc = nk_process_spawn(program, argument);
    if (proc == NULL)
      return -1;
    return proc->pid;
  }

  if (nr == SYSCALL_WAIT) {
    nk_process_t *target_proc = get_process_pid(a);
    if (target_proc == NULL)
      return -1;

    // Note, there are some hideous race conditions that could happen between
    // the call to `get_process_id` and here relating to multiple processes
    // calling SYSCALL_WAIT at the same time. It isn't really a problem in this
    // toy process abstraction, due to it being single-threaded and there not
    // being a real way to communicate between processes. In a real system, you
    // would need to guarentee that someone isn't currently waiting on this
    // thread, as they could free the threads state between when you find the
    // process and when you call wait. This is a *really* hard thing to solve
    // correctly (and robustly), so we won't quite do it here.
    // If you are interested to see how Nautilus fixes it (with normal threads)
    // check out the implementation of `handle_special_switch` and the exit
    // function (which avoid the use of the stack entirely.)
    return nk_process_wait(target_proc);
  }

  if (nr == SYSCALL_VALLOC) {
    // Allocate `a` pages in the processes' address space, and return a pointer
    // to the start of the new region. This region should be anonoymously
    // allocated to zero pages.
    return process_valloc(proc, a);
  }

  if (nr == SYSCALL_VFREE) {
    // TODO
    return -1;
  }

  if (nr == SYSCALL_YIELD) {
    // Simply forward the request to the scheduler.
    nk_sched_yield(NULL);
    return 0;
  }

  if (nr == SYSCALL_OPEN) {
    // UNSAFE! Bad to read directly from userspace like this.
    char *path = (char *)a;

    int fd_nr = -1;
    for (int i = 0; i < PROCESS_FD_TABLE_SIZE; i++) {
      if (proc->open_files[i] == FS_BAD_FD) {
        fd_nr = i;
        break;
      }
    }
    // no open files. Just return -1 ("failed to open")
    if (fd_nr == -1)
      return -1;

    nk_fs_fd_t fd = nk_fs_open(path, b /* b is the flags*/, 0666);
    if (fd == FS_BAD_FD) {
      return -1;
    }

    proc->open_files[fd_nr] = fd;
    return fd_nr;
  }

  if (nr == SYSCALL_CLOSE) {
    nk_fs_fd_t fd = process_get_fd(proc, a);
    if (fd == FS_BAD_FD)
      return -1;
    nk_fs_close(fd);
    proc->open_files[a] = FS_BAD_FD;
    return 0;
  }

  if (nr == SYSCALL_READ) {
    int fd_nr = (int)a;
    void *dst = (void *)b;
    long size = (long)c;
    nk_fs_fd_t fd = process_get_fd(proc, fd_nr);
    if (fd == FS_BAD_FD)
      return -1;
    // Note: this is *very* dangerous. We are letting random parts of the
    // kernel, namely the filesystem, write to an arbitrary pointer the user has
    // provided.
    long n = nk_fs_read(fd, dst, size);
    return n;
  }

  if (nr == SYSCALL_WRITE) {
    int fd_nr = (int)a;
    void *dst = (void *)b;
    long size = (long)c;
    nk_fs_fd_t fd = process_get_fd(proc, fd_nr);
    if (fd == FS_BAD_FD)
      return -1;
    // Note: this is *very* dangerous. We are letting random parts of the
    // kernel, namely the filesystem, write to an arbitrary pointer the user has
    // provided.
    long n = nk_fs_write(fd, dst, size);
    return n;
  }

  printk("Unknown system call. Nr=%d, args=%d,%d,%d\n", nr, a, b, c);

  return 0;
}

/**
 * This is the first function to be run when executing userspace code. You may
 * be asking, "What? This is in the kernel! How can this be user code???". Yes,
 * this code executes in the kernel, but it is required to do so in order to
 * setup some state before starting the userspace code. You'll notice the first
 * thing it does is allocate a stack for the user code to use. It then sets up a
 * user frame and starts the code from there. From then on out, this thread is
 * in userspace, or is handling an exception/trap/interrupt in kernel mode.
 */
static void process_bootstrap_and_start(void *input, void **output) {
  nk_thread_t *t = get_cur_thread();
  nk_process_t *p = get_process(t);
  nk_thread_name(t, p->program);
  nk_process_switch(t);

  // Attach the address space to this thread.
  nk_aspace_move_thread(p->aspace);

  // allocate one page for the stack
  addr_t stack_top = process_valloc(p, 1);

  // Initialize the registers for this thread (mainly RIP and RSP)
  struct user_frame frame;
  memset(&frame, 0, sizeof(frame));
  frame.rip = (uint64_t)USER_ASPACE_START;    // userspace code
  frame.rsp = (uint64_t)stack_top + 4096 - 8; // userspace stack
  frame.cs = (SEG_UCODE << 3) | 3;            // user code segment in ring 3
  frame.ss = (SEG_UDATA << 3) | 3;            // user data segment in ring 3
  frame.rflags = 0x00000200;                  // enable interrupts

  // Call the userspace code
  user_start((void *)&frame, SEG_UDATA);

  panic("'somehow, userspace returned'");
}

/*
 * Setup a process' address space. The main thing that happens here is we
 * create a paging aspace, attach it to the process and the main thread, then
 * add a region for the kernel to be mapped into, then we create a region
 * for the stack and the user binary.
 *
 * Returns: 0 on success, -1 on failure.
 *     The aspace and all intermediate structs will be
 *      cleaned up here if anything fails.
 */
static int setup_process_aspace(nk_process_t *proc,
                                nk_aspace_characteristics_t *aspace_chars) {
  // The region we map later on.
  nk_aspace_region_t region;
  // A buffer to storethe formatted aspace name
  char aspace_name[32];

  // allocate an aspace for the process. We do this by asking the aspace
  // subsystem (which abstracts all the concepts of an aspace) to create a
  // "paging" aspace. Implementing that aspace is will be what you will do in
  // the lab.
  sprintf(aspace_name, "proc%d", proc->pid);
  proc->aspace = nk_aspace_create("paging", aspace_name, aspace_chars);
  if (proc->aspace == NULL) {
    ERROR("Failed to allocate aspace for process!\n");
    goto clean_up;
  }

  // create a 1-1 region mapping all of physical memory so that
  // the kernel can work when the process is active
  region.va_start = 0;
  region.pa_start = 0;
  // Map 4 GB
  region.len_bytes = 0x100000000UL;
  // set protections for kernel
  // use EAGER to tell paging implementation that it needs to build all these
  // PTs right now
  region.protect.flags = NK_ASPACE_READ | NK_ASPACE_WRITE | NK_ASPACE_EXEC |
                         NK_ASPACE_PIN | NK_ASPACE_EAGER | NK_ASPACE_KERN;

  // Add the region to the aspace, which should map the memory immediately.
  if (nk_aspace_add_region(proc->aspace, &region)) {
    ERROR("failed to add initial eager region to address space\n");
    goto clean_up;
  }
  return 0;

clean_up:
  // Delete the aspace if we ended up creating it.
  if (proc->aspace != NULL)
    nk_aspace_destroy(proc->aspace);
  return -1;
}

static void process_free(nk_process_t *process) {
  PTABLE_LOCK_CONF;

  // Free the aspace
  nk_aspace_destroy(process->aspace);

  // Remove the process from the process list
  PTABLE_LOCK();
  list_del_init(&process->ptable_list_node);
  PTABLE_UNLOCK();

  // Close all the open files
  for (int i = 0; i < PROCESS_FD_TABLE_SIZE; i++) {
    if (process->open_files[i] != FS_BAD_FD) {
      nk_fs_close(process->open_files[i]);
      process->open_files[i] = FS_BAD_FD;
    }
  }

  // Free the process state
  free(process);
}

nk_process_t *nk_process_spawn(const char *program, const char *argument) {
  PTABLE_LOCK_CONF;
  nk_process_t *proc = NULL;
  nk_thread_t *thread;
  struct nk_fs_stat stat;
  nk_fs_fd_t fd;

  // Check that the program exists
  if (nk_fs_stat((char *)program, &stat) < 0) {
    ERROR("No such program exists (%s)\n", program);
    goto clean_up;
  }

  // Then, we query the aspace implementation to make
  // sure it has a paging implmentation for us.
  nk_aspace_characteristics_t aspace_chars;
  if (nk_aspace_query("paging", &aspace_chars) != 0) {
    ERROR("No paging aspace implementation available\n");
    goto clean_up;
  }

  // Allocate the memory for the process structure
  proc = malloc(sizeof(*proc));
  if (!proc) {
    ERROR("cannot allocate process\n");
    goto clean_up;
  }
  // Make sure the process structure is zeroed
  memset(proc, 0, sizeof(*proc));

  // spawn the first thread
  if (nk_thread_create(process_bootstrap_and_start, (void *)argument, NULL, 0,
                       4096, &proc->main_thread, CPU_ANY) < 0) {
    ERROR("cannot allocate main thread for process\n");
    goto clean_up;
  }

  // now that we have allocated the process and it's first thread, we should be
  // good to go! First, we'll configure the process' name and update the main
  // thread's name to match.
  strncpy(proc->program, program, sizeof(proc->program) - 1);
  nk_thread_name(&proc->main_thread, proc->program);

  // Allocate a pid and insert this process into the process table.
  PTABLE_LOCK();
  // allocate a pid
  proc->pid = proc_next_pid++;
  // Add the process to the table.
  list_add_tail(&proc->ptable_list_node, &ptable_list);
  PTABLE_UNLOCK()

  if (setup_process_aspace(proc, &aspace_chars)) {
    ERROR("Failed to initialize process' address space\n");
    goto clean_up;
  }
  // initialize all the files
  for (int i = 0; i < PROCESS_FD_TABLE_SIZE; i++) {
    proc->open_files[i] = FS_BAD_FD;
  }

  // Next, open the binary file and map it into memory with NK_ASPACE_FILE
  fd = nk_fs_open((char *)program, O_RDONLY, 0);
  nk_aspace_region_t region;
  region.va_start = USER_ASPACE_START;
  region.pa_start = 0;
  region.len_bytes = round_up(stat.st_size, 0x1000); // map the
  region.file = fd;
  region.protect.flags = NK_ASPACE_READ | NK_ASPACE_WRITE | NK_ASPACE_EXEC |
                         NK_ASPACE_PIN | NK_ASPACE_EAGER | NK_ASPACE_FILE;
  nk_aspace_add_region(proc->aspace, &region);
  // cache this in the last file descriptor so it gets closed *after* the aspace
  // is destroyed when the process exits.
  proc->open_files[PROCESS_FD_TABLE_SIZE - 1] = fd;

  // When a process requests new pages, allocate them here
  // then bump it up (with a gap for "protection")
  proc->next_palloc = (off_t)USER_ASPACE_START + region.len_bytes + 4096;

  // Configure some fields on the processes' main thread.
  thread = proc->main_thread;
  // Make sure the thread prints to the right console
  thread->vc = get_cur_thread()->vc;
  // And make sure the thread knows what process it is a part of.
  thread->process = proc;

  // Make sure everything we just did sticks by performing a memory fence. This
  // guarentees that wherever the thread is run in the `nk_thread_run`
  // invocation below, all the state of the process and it's main thread are
  // consistent on the whole system. You really don't need to worry about this
  // in QEMU, but on a real system this is a nightmare of a problem to debug :^)
  asm volatile("mfence" ::: "memory");

  // kickoff main thread
  nk_thread_run(proc->main_thread);

  return proc;

  // A lovely label which we `goto` when we need to cleanup the process we were
  // creating if it fails at any point.
clean_up:
  if (proc != NULL) {
    // Delete the main thread
    if (proc->main_thread != NULL)
      nk_thread_destroy(proc->main_thread);
    process_free(proc);
  }
  return NULL;
}

// An implementation of process waiting. Most of the heavy lifting of this
// function piggy-backs on nk_join, which does the hard work of dealing with the
// scheduler.
int nk_process_wait(nk_process_t *process) {

  // Join the thread, which transitively frees it's state
  nk_join(process->main_thread, NULL);
  process_free(process);
  return 0;
}
