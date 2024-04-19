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
 * Copyright (c) 2023, Nick Wanninger <ncw@u.northwestern.edu>
 *                     Kevin Mendoza Tudares <mendozatudares@u.northwestern.edu>
 *                     Kevin Hayes <kjhayes@u.northwestern.edu>
 * Copyright (c) 2023, The V3VEE Project  <http://www.v3vee.org>
 *                     The Hobbes Project <http://xstack.sandia.gov/hobbes>
 * All rights reserved.
 *
 * Authors: Nick Wanninger <ncw@u.northwestern.edu>
 *          Kevin Mendoza Tudares <mendozatudares@u.northwestern.edu>
 *          Kevin Hayes <kjhayes@u.northwestern.edu>
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "LICENSE.txt".
 */

#pragma once

#include <nautilus/nautilus.h>

//typedef struct excp_entry_state excp_entry_t;

typedef struct mmap_info mmap_info_t;
typedef struct mem_map_entry mem_map_entry_t;

struct naut_info;
struct nk_io_mapping;

/* arch specific */
void arch_enable_ints(void);
void arch_disable_ints(void);
int arch_ints_enabled(void);

int arch_smp_early_init(struct naut_info *naut);
int arch_numa_init(struct sys_info *sys);

nk_irq_t arch_xcall_irq(void);

void arch_detect_mem_map(mmap_info_t *mm_info, mem_map_entry_t *memory_map,
                         unsigned long mbd);
void arch_reserve_boot_regions(unsigned long mbd);

uint32_t arch_cycles_to_ticks(uint64_t cycles);
uint32_t arch_realtime_to_ticks(uint64_t ns);
uint64_t arch_realtime_to_cycles(uint64_t ns);
uint64_t arch_cycles_to_realtime(uint64_t cycles);

void arch_update_timer(uint32_t ticks, nk_timer_condition_t cond);
void arch_set_timer(uint32_t ticks);
int arch_read_timer(void);
int arch_timer_handler(struct nk_irq_action *action, struct nk_regs *regs, void *state);

uint64_t arch_read_timestamp(void);

void *arch_read_sp(void);
void arch_relax(void);
void arch_halt(void);

int arch_little_endian(void);

int arch_atomics_enabled(void);

int arch_handle_io_map(struct nk_io_mapping *mapping);
int arch_handle_io_unmap(struct nk_io_mapping *mapping);

void arch_print_regs(struct nk_regs *r);
void* arch_instr_ptr_reg(struct nk_regs *regs);

#ifdef NAUT_CONFIG_ARCH_X86
#include <arch/x64/arch.h>
#elif NAUT_CONFIG_ARCH_RISCV
#include <arch/riscv/arch.h>
#elif NAUT_CONFIG_ARCH_ARM64
#include <arch/arm64/arch.h>
#else
#error "Unsupported architecture"
#endif

#ifndef NAUT_CONFIG_SPARSE_IRQ
#ifndef MAX_IRQ_NUM
#error "Architecture did not specify MAX_IRQ_NUM or CONFIG_SPARSE_IRQ!"
#endif
#endif

