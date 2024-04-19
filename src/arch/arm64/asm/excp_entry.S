/*
 * This file is part of the Nautilus AeroKernel developed
 * by the Constellation, Interweaving, Hobbes, and V3VEE
 * Projects with funding from the United States National
 * Science Foundation and the Department of Energy.
 *
 * The V3VEE Project is a joint project between Northwestern University
 * and the University of New Mexico.  The Hobbes Project is a collaboration
 * led by Sandia National Laboratories that includes several national
 * laboratories and universities. The Interweaving Project is a
 * joint project between Northwestern University and Illinois Institute
 * of Technology.   The Constellation Project is a joint project
 * between Northwestern University, Carnegie Mellon University,
 * and Illinois Institute of Technology.
 * You can find out more at:
 * http://www.v3vee.org
 * http://xstack.sandia.gov/hobbes
 * http://interweaving.org
 * http://constellation-project.net
 *
 * Copyright (c) 2023, Kevin Hayes <kjhayes@u.northwestern.edu>
 * Copyright (c) 2023, The V3VEE Project  <http://www.v3vee.org>
 *                     The Hobbes Project <http://xstack.sandia.gov/hobbes>
 * All rights reserved.
 *
 * Authors: Kevin Hayes <kjhayes@u.northwestern.edu>
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "LICENSE.txt".
 */

.section .text

.include "arch/arm64/asm/save_regs.S"

.macro NESTABLE_EXCP_ENTRY

  sub sp, sp, 0x20

  // Store interrupt corruptible registers
  mrs x1, SPSR_EL1
  mrs x2, ELR_EL1
  stp x1, x2, [sp, 0x00]

  // Get extra info about the interrupt
  mrs x1, ESR_EL1
  mrs x2, FAR_EL1
  stp x1, x2, [sp, 0x10]

  mov x1, sp

  // Increment exception depth (if non-null)
  mrs x2, TPIDR_EL1
  cmp x2, 0
  b.eq 1f
  ldrh w3, [x2, 8]
  add w3, w3, 1
  strh w3, [x2, 8]
1: 

.endm

.macro NESTABLE_EXCP_EXIT

  // Restore interrupt registers
  ldp x0, x1, [sp, 0x00]
  msr SPSR_EL1, x0
  msr ELR_EL1, x1

  add sp, sp, 0x20

.endm

.macro DEC_EXCP_DEPTH
  mrs x2, TPIDR_EL1
  cmp x2, 0
  b.eq 1f
  ldrh w3, [x2, 8]
  sub w3, w3, 1
  strh w3, [x2, 8]
1:
.endm

.extern route_interrupt
.extern nk_thread_switch_intr_entry

.global lower_interrupt_entry
.global lower_fast_interrupt_entry
lower_interrupt_entry:
lower_fast_interrupt_entry:
  SAVE_CALLER_GPRS 
  mov x0, sp // Store nk_regs*
#ifdef NAUT_CONFIG_FPU_SAVE_IRQ_EXCP
  SAVE_CALLER_FPRS
#endif

  NESTABLE_EXCP_ENTRY 

  mov x2, 0
  bl route_interrupt

  DEC_EXCP_DEPTH

  cbz x0, 1f
  bl nk_thread_switch_intr_entry
1:

  NESTABLE_EXCP_EXIT
#ifdef NAUT_CONFIG_FPU_SAVE_IRQ_EXCP
  RESTORE_CALLER_FPRS
#endif
  RESTORE_CALLER_GPRS
  eret

.global same_interrupt_entry
.global same_fast_interrupt_entry
same_fast_interrupt_entry:
same_interrupt_entry:
  SAVE_CALLER_GPRS
  mov x0, sp // Store nk_regs*
#ifdef NAUT_CONFIG_FPU_SAVE_IRQ_EXCP
  SAVE_CALLER_FPRS
#endif

  NESTABLE_EXCP_ENTRY

  mov x2, 1
  bl route_interrupt

  DEC_EXCP_DEPTH

  cbz x0, 1f 
  bl nk_thread_switch_intr_entry
1:
  
  NESTABLE_EXCP_EXIT

#ifdef NAUT_CONFIG_FPU_SAVE_IRQ_EXCP
  RESTORE_CALLER_FPRS
#endif
  RESTORE_CALLER_GPRS
same_interrupt_end:
  eret

.extern route_exception

.global lower_serror_entry
.global lower_exception_entry
lower_serror_entry:
  SAVE_CALLER_GPRS
  mov x0, sp // Store nk_regs*
#ifdef NAUT_CONFIG_FPU_SAVE_IRQ_EXCP
  SAVE_CALLER_FPRS
#endif

  NESTABLE_EXCP_ENTRY
  mov x3, 0
  b __lower_excp_entry_common
lower_exception_entry:
  SAVE_CALLER_GPRS
  mov x0, sp // Store nk_regs*
#ifdef NAUT_CONFIG_FPU_SAVE_IRQ_EXCP
  SAVE_CALLER_FPRS
#endif

  NESTABLE_EXCP_ENTRY
  mov x3, 1

__lower_excp_entry_common:

  mov x2, 0
  bl route_exception

  DEC_EXCP_DEPTH

  NESTABLE_EXCP_EXIT
#ifdef NAUT_CONFIG_FPU_SAVE_IRQ_EXCP
  RESTORE_CALLER_FPRS
#endif
  RESTORE_CALLER_GPRS
  eret

.global same_serror_entry
.global same_exception_entry
same_serror_entry:
  SAVE_CALLER_GPRS
  mov x0, sp // Store nk_regs*
#ifdef NAUT_CONFIG_FPU_SAVE_IRQ_EXCP
  SAVE_CALLER_FPRS
#endif

  NESTABLE_EXCP_ENTRY
  mov x3, 0
  b __same_excp_entry_common
same_exception_entry:
  SAVE_CALLER_GPRS
  mov x0, sp // Store nk_regs*
#ifdef NAUT_CONFIG_FPU_SAVE_IRQ_EXCP
  SAVE_CALLER_FPRS
#endif

  NESTABLE_EXCP_ENTRY
  mov x3, 1

__same_excp_entry_common:
  mov x2, 1
  bl route_exception

  DEC_EXCP_DEPTH

  NESTABLE_EXCP_EXIT
#ifdef NAUT_CONFIG_FPU_SAVE_IRQ_EXCP
  RESTORE_CALLER_FPRS
#endif
  RESTORE_CALLER_GPRS
  eret

