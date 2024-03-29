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
 * http://xtack.sandia.gov/hobbes
 *
 * Copyright (c) 2015, Kyle C. Hale <kh@u.northwestern.edu>
 * Copyright (c) 2015, The V3VEE Project  <http://www.v3vee.org> 
 *                     The Hobbes Project <http://xstack.sandia.gov/hobbes>
 * All rights reserved.
 *
 * Author: Kyle C. Hale <kh@u.northwestern.edu>
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "LICENSE.txt".
 */
#include <nautilus/nautilus.h>
#include <nautilus/random.h>
#include <nautilus/cpu.h>
#include <nautilus/percpu.h>
#include <nautilus/smp.h>
#include <nautilus/spinlock.h>
#include <nautilus/random.h>
#include <nautilus/mm.h>

#ifdef NAUT_CONFIG_ARCH_X86
#include <dev/apic.h>
#endif


void
nk_rand_seed (uint64_t seed) {
    struct nk_rand_info * rand = per_cpu_get(rand);
    uint8_t flags = spin_lock_irq_save(&rand->lock);
    rand->xi   = seed;
    rand->seed = seed;
    rand->n    = 0;
    spin_unlock_irq_restore(&rand->lock, flags);
}


/* 
 * side effect: increments counter
 * NOTE: this function assumes that the random lock
 * is held when coming in
 */
void
nk_rand_set_xi (uint64_t xi) 
{
    struct nk_rand_info * rand = per_cpu_get(rand);
    rand->xi = xi;
    rand->n++;
}


#define _AB(x,n) ((char)(((x) >> (n)) & 0xff))
/* 
 * To get something appearing random, 
 * we fold the APIC IRRs together and 
 * then combine them with the cycle count
 *
 * this is likely pretty bogus, but 
 * we don't need any kind of security for now,
 * just something that appears random
 */
static char
get_rand_byte (void) 
{
// RISCV HACK
#if defined(NAUT_CONFIG_ARCH_X86)
    struct apic_dev * apic = per_cpu_get(apic);
    struct nk_rand_info * rand = per_cpu_get(rand);
    uint64_t cycles;
    uint32_t val;
    char b =  0xff;
    char b2 = 0;
    uint8_t i;


    for (i = 0; i < 8; i++) {
        #ifdef NAUT_CONFIG_ARCH_RISCV
        val = read_csr(sip) ^ (rand->seed & 0xffffffff);
        #else
        val = apic_read(apic, APIC_GET_IRR(i)) ^ (rand->seed & 0xffffffff);
        #endif
        b ^= ~(_AB(val, 0) ^
               _AB(val, 1) ^
               _AB(val, 2) ^
               _AB(val, 3));
    }

    cycles = rdtsc();

    for (i = 0; i < 8; i++) {
        b2 ^= _AB(cycles, i);
    }

    return b + b2;
#elif defined(NAUT_CONFIG_ARCH_ARM64) || defined(NAUT_CONFIG_ARCH_RISCV)
    // This 100% not pseudo-random I just need something
    // that could potentially return any value 0x00-0xff
    return (char)(arch_read_timestamp());
#else
    // TODO This can brick the scheduler (If it's not random, it atleast needs to change over time) -KJH
    return 0;
#endif
}


void
nk_get_rand_bytes (uint8_t * buf, unsigned len)
{
    if (!buf) {
        return;
    }

    while (len--) {
        *buf++ = get_rand_byte();
    }
}


int
nk_rand_init (struct cpu * cpu) 
{
    cpu->rand = malloc(sizeof(struct nk_rand_info));
    if (!cpu->rand) {
        ERROR_PRINT("Could not allocate CPU random info\n");
        return -1;
    }
    memset(cpu->rand, 0, sizeof(struct nk_rand_info));

    spinlock_init(&cpu->rand->lock);

    return 0;
}
