#include <nautilus/nautilus.h>
#include <nautilus/shell.h>
#include <nautilus/user.h>


#define ERROR(fmt, args...) ERROR_PRINT("userspace: " fmt, ##args)


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

nk_process_t *nk_process_create(const char *program, const char *argument) {
  PTABLE_LOCK_CONF;

  // allocate the memory for the process
  nk_process_t *proc = malloc(sizeof(*proc));
  if (!proc) {
    ERROR("cannot allocate process\n");
    return NULL;
  }

  // spawn the first thread
  if (nk_thread_create(process_run_user, (void *)argument, NULL, 0, 4096,
                       &proc->main_thread, CPU_ANY) < 0) {
    ERROR("cannot allocate main thread for process\n");
    free(proc);
    return NULL;
  }

  memset(proc, 0, sizeof(*proc));

  // configure the process' name
  strncpy(proc->program, program, sizeof(proc->program) - 1);
  nk_thread_name(&proc->main_thread, proc->program);

  PTABLE_LOCK();
  // allocate a pid
  proc->pid = proc_next_pid++;

#if 0
  // insert into the ptable
  list_add_tail(&a->aspace_list_node, &aspace_list);
  PTABLE_UNLOCK();

  // allocate an aspace for the process
  char aspace_name[32];
  sprintf("pid%d\n", proc->pid);
  nk_aspace_characteristics_t chars;
  if (!nk_aspace_query("paging", &chars)) {

    // TODO error
    return NULL;
  }
  // allocate the aspace
  proc->aspace = nk_aspace_create("paging", aspace_name, &chars);



  // configure some stuff on the main thread
  nk_thread_t *t = proc->main_thread;
  t->vc = get_cur_thread()->vc;

  // set the main thread's address space
  t->aspace = proc->aspace;

  // kickoff main thread
  nk_thread_run(proc->main_thread);
#endif

  return proc;
}

// mark the process as exited, and exit the main thread
// (this MUST be called from the main thread)
void nk_process_exit(nk_process_t *process) {}

int nk_process_wait(nk_process_t *process) {
	return -1;
}
