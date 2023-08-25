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
 * Author: Kyle C. Hale <kh@u.northwestern.edu>
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "LICENSE.txt".
 */
#ifndef __BARRIER_H__
#define __BARRIER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <nautilus/spinlock.h>
#include <nautilus/atomic.h>

#define NK_BARRIER_LAST 1

typedef struct nk_barrier nk_barrier_t;

struct nk_barrier {
    spinlock_t lock; /* SLOW */
    
    unsigned remaining;
    unsigned init_count;

    uint8_t  active; /* used for core barriers */

    uint8_t pad[52];

    /* this is on another cache line (Assuming 64b) */
    volatile __attribute__((aligned(4))) unsigned notify;
} __attribute__ ((packed)) __attribute((aligned(64)));

int nk_barrier_init (nk_barrier_t * barrier, uint32_t count);
int nk_barrier_destroy (nk_barrier_t * barrier);
int nk_barrier_wait (nk_barrier_t * barrier);
void nk_barrier_test(void);

/* CORE barriers */
int nk_core_barrier_raise(void);
int nk_core_barrier_lower(void);
int nk_core_barrier_wait(void); // waits on all other cores to arrive at barrier
int nk_core_barrier_arrive(void); // arrive (and wait) at the core barrier


// counting barriers

typedef struct nk_counting_barrier {
    // number of threads that must arrive at the barrier
    // note that this can only be set when there is no barrier in progress
    uint64_t      size;
    uint64_t      count[2];
    uint64_t      cur;
} nk_counting_barrier_t;
    

static inline void nk_counting_barrier_init(nk_counting_barrier_t *b, uint64_t size)
{
    b->size=size; b->count[0]=b->count[1]=0; b->cur=0;
}

static inline void nk_counting_barrier(volatile nk_counting_barrier_t *b)
{
    uint64_t old;
    volatile uint64_t *curp = &(b->cur);
    long mycur = *curp;
    volatile uint64_t *countp = &(b->count[mycur]);

    old = atomic_add(*countp,1);

    if (old==(b->size-1)) {
        // I'm the last to the party
	// We need to be sure that these operations occur in order 
	// and are fully visible in order
    #ifdef NAUT_CONFIG_ARCH_RISCV
			*curp ^= 0x1;
			__asm__ __volatile__ ("fence.i" : : : "memory");
			*countp = 0;
			__asm__ __volatile__ ("fence.i" : : : "memory");
    #elif NAUT_CONFIG_ARCH_ARM64
			*curp ^= 0x1;
                        __sync_synchronize();
			*countp = 0;
                        __sync_synchronize();
    #elif NAUT_CONFIG_ARCH_X86
			*curp ^= 0x1;
			__asm__ __volatile__ ("mfence" : : : "memory");
			*countp = 0;
			__asm__ __volatile__ ("mfence" : : : "memory");
		#else 	
			#error "unsupported arch"
    #endif
    } else {
        // k1om compiler does not know what "volatile" means
        // hence this hand-coding.
    #ifdef NAUT_CONFIG_ARCH_RISCV
	while ( ({ __asm__ __volatile__( "ld %0, %1" : "=r"(old) : "m"(*countp) : ); old; }) ) {
	    __asm__ __volatile__ ("nop");
	}
    #elif NAUT_CONFIG_ARCH_ARM64
			// TODO(arm64): mfence. Do this with __sync_synchronize();
        __sync_synchronize();
    #elif NAUT_CONFIG_ARCH_X86
	while ( ({ __asm__ __volatile__( "movq %1, %0" : "=r"(old) : "m"(*countp) : ); old; }) ) {
	    __asm__ __volatile__ ("pause");
	}
		#else 	
			#error "unsupported arch"
    #endif
    }
}

#ifdef __cplusplus
}
#endif

#endif /* !__BARRIER_H__ */
