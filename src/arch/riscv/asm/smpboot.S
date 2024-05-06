#include <arch/riscv/asm/lowlevel.h>

#define SIE_SEIE (1L << 9)  // external
#define SIE_STIE (1L << 5)  // timer
#define SIE_SSIE (1L << 1)  // software

ENTRY(init_smp_boot)

    # disable supervisor address translation and protection (paging), since we'd identity map anyways
    csrw    satp, 0

    # flush tlb and whatnot
    sfence.vma zero, zero

    ld sp, secondary_core_stack

    # enable supervisor interrupts
    li      t0, SIE_SEIE | SIE_STIE | SIE_SSIE
    csrs    sie, t0

    ld t0, second_entry_boot_ptr
    jalr t0

    mv fp, a0            // set our new frame pointer
    mv sp, a0            // switch stacks

    ld t0, second_entry_thread_ptr
    jalr t0

    // We should never return
    j .

.balign 8
second_entry_boot_ptr: .quad secondary_entry_boot_stack

.balign 8
second_entry_thread_ptr: .quad secondary_entry_threaded


.globl end_smp_boot
end_smp_boot:

/*
 * we come in with two arguments:
 * a0 = new rsp
 * a1 = new rbp
 * a2 = cpu_ptr
 * we leave with a new stack
 */
ENTRY(smp_ap_stack_switch)
     mv fp, a1            // set our new frame pointer
     mv sp, a0            // switch stacks
     mv a0, a2            // return the old cpu pointer
     jr ra                // jump back to calling routine
