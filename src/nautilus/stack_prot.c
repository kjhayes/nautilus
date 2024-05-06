
#include <nautilus/nautilus.h>
#include <nautilus/thread.h>

uintptr_t __stack_chk_guard = 0x0123456789abcdef;

void
__stack_chk_fail(void) {
    struct nk_thread *thr = get_cur_thread();
    panic("CPU (%u): Failed Stack Check at Runtime (RA: %p, TID: %llu, thread: \"%s\")!\n",
            my_cpu_id(), __builtin_return_address(0),
            thr->tid, thr->name != NULL ? thr->name : "");
    while(1);
}

