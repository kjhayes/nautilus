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
 * Copyright (c) 2015, Kyle C. Hale <khale@cs.iit.edu>
 * Copyright (c) 2015, The V3VEE Project  <http://www.v3vee.org>
 *                     The Hobbes Project <http://xstack.sandia.gov/hobbes>
 * All rights reserved.
 *
 * Author: Kyle C. Hale <khale@cs.iit.edu>
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "LICENSE.txt".
 */
#ifdef __CPU_H__

struct nk_regs {
  uint64_t ra; /* x1: Return address */
  uint64_t sp; /* x2: Stack pointer */
  uint64_t gp; /* x3: Global pointer */
  uint64_t tp; /* x4: Thread Pointer */
  uint64_t t0; /* x5: Temp 0 */
  uint64_t t1; /* x6: Temp 1 */
  uint64_t t2; /* x7: Temp 2 */
  uint64_t s0; /* x8: Saved register / Frame Pointer */
  uint64_t s1; /* x9: Saved register */
  uint64_t a0; /* Arguments, you get it :) */
  uint64_t a1;
  uint64_t a2;
  uint64_t a3;
  uint64_t a4;
  uint64_t a5;
  uint64_t a6;
  uint64_t a7;
  uint64_t s2; /* More Saved registers... */
  uint64_t s3;
  uint64_t s4;
  uint64_t s5;
  uint64_t s6;
  uint64_t s7;
  uint64_t s8;
  uint64_t s9;
  uint64_t s10;
  uint64_t s11;
  uint64_t t3; /* More temporaries */
  uint64_t t4;
  uint64_t t5;
  uint64_t t6;

  /* Exception PC */
  uint64_t sepc;    /* 31 */
  uint64_t status;  /* 32 */
  uint64_t tval;    /* 33 */
  uint64_t cause;   /* 34 */
  uint64_t scratch; /* 35 */
  /* Missing floating point registers in the kernel trap frame */
};

#define pause()       asm volatile("nop");

#define PAUSE_WHILE(x) \
    while ((x)) { \
        pause(); \
    }

#define mbarrier()    asm volatile("fence.i":::"memory")

#define BARRIER_WHILE(x) \
    while ((x)) { \
        mbarrier(); \
    }

#define read_csr(name)                         \
  ({                                           \
    uint64_t x;                             \
    asm volatile("csrr %0, " #name : "=r"(x)); \
    x;                                         \
  })

#define write_csr(csr, val)                                             \
  ({                                                                    \
    unsigned long __v = (unsigned long)(val);                           \
    __asm__ __volatile__("csrw " #csr ", %0" : : "rK"(__v) : "memory"); \
  })


#define set_csr(csr, val)                                               \
  ({                                                                    \
    unsigned long __v = (unsigned long)(val);                           \
    __asm__ __volatile__("csrs " #csr ", %0" : : "rK"(__v) : "memory"); \
  })


#define clear_csr(csr, val)                                               \
  ({                                                                    \
    unsigned long __v = (unsigned long)(val);                           \
    __asm__ __volatile__("csrc " #csr ", %0" : : "rK"(__v) : "memory"); \
  })

static inline uint8_t
inb (uint64_t addr)
{
    uint8_t ret;
    asm volatile ("lb  %[_r], 0(%[_a])"
                  : [_r] "=r" (ret)
                  : [_a] "r" (addr));
    return ret;
}


static inline uint16_t
inw (uint64_t addr)
{
    uint16_t ret;
    asm volatile ("lh  %[_r], 0(%[_a])"
                  : [_r] "=r" (ret)
                  : [_a] "r" (addr));
    return ret;
}


static inline uint32_t
inl (uint64_t addr)
{
    uint32_t ret;
    asm volatile ("lw  %[_r], 0(%[_a])"
                  : [_r] "=r" (ret)
                  : [_a] "r" (addr));
    return ret;
}


static inline void
outb (uint8_t val, uint64_t addr)
{
    asm volatile ("sb  %[_v], 0(%[_a])"
                  :
                  : [_a] "r" (addr),
                    [_v] "r" (val));
}


static inline void
outw (uint16_t val, uint64_t addr)
{
    asm volatile ("sh  %[_v], 0(%[_a])"
                  :
                  : [_a] "r" (addr),
                    [_v] "r" (val));
}

static inline void
outl (uint32_t val, uint64_t addr)
{
    asm volatile ("sw  %[_v], 0(%[_a])"
                  :
                  : [_a] "r" (addr),
                    [_v] "r" (val));
}

static inline uint64_t __attribute__((always_inline))
rdtsc (void)
{
    uint64_t x;
    asm volatile("csrr %0, time" : "=r" (x) );
    return x;
}

#define rdtscp() rdtsc

static inline uint64_t
read_rflags (void)
{
    uint64_t x;
    asm volatile("csrr %0, sstatus" : "=r" (x) );
    return x;
}

#define SSTATUS_SIE (1L << 1)
#define RFLAGS_IF   SSTATUS_SIE

static inline void
sti (void)
{
    uint64_t x = read_rflags() | SSTATUS_SIE;
    asm volatile("csrw sstatus, %0" : : "r" (x));
}


static inline void
cli (void)
{
    uint64_t x = read_rflags() & ~SSTATUS_SIE;
    asm volatile("csrw sstatus, %0" : : "r" (x));
}

static inline void
halt (void)
{
    asm volatile ("wfi");
}


static inline void
invlpg (unsigned long addr)
{

}


static inline void
wbinvd (void)
{

}


static inline void clflush(void *ptr)
{

}

static inline void clflush_unaligned(void *ptr, int size)
{

}

/**
 * Flush all non-global entries in the calling CPU's TLB.
 *
 * Flushing non-global entries is the common-case since user-space
 * does not use global pages (i.e., pages mapped at the same virtual
 * address in *all* processes).
 *
 */
static inline void
tlb_flush (void)
{
    asm volatile("sfence.vma zero, zero");
}

static inline void io_delay(void)
{
    // const uint16_t DELAY_PORT = 0x80;
    // outb(0, DELAY_PORT);
    pause();
}

static void udelay(uint_t n) {
    while (n--){
        io_delay();
    }
}

/* Macros for pointer/register sizes */
#define PTRLOG 3
#define SZREG 8
#define REG_S sd
#define REG_L ld
#define REG_SC sc.d
#define ROFF(N, R) N* SZREG(R)

/* Status register flags */
#define SR_SIE		(0x00000002UL) /* Supervisor Interrupt Enable */
#define SR_MIE		(0x00000008UL) /* Machine Interrupt Enable */
#define SR_SPIE		(0x00000020UL) /* Previous Supervisor IE */
#define SR_MPIE		(0x00000080UL) /* Previous Machine IE */
#define SR_SPP		(0x00000100UL) /* Previously Supervisor */
#define SR_MPP		(0x00001800UL) /* Previously Machine */
#define SR_SUM		(0x00040000UL) /* Supervisor User Memory Access */

#define CSR_SSTATUS		0x100
#define CSR_SIE			0x104
#define CSR_STVEC		0x105
#define CSR_SCOUNTEREN		0x106
#define CSR_SSCRATCH		0x140
#define CSR_SEPC		0x141
#define CSR_SCAUSE		0x142
#define CSR_STVAL		0x143
#define CSR_SIP			0x144
#define CSR_SATP		0x180

#define MSTATUS_MPP_MASK (3L << 11)  // previous mode.
#define MSTATUS_MPP_M (3L << 11)
#define MSTATUS_MPP_S (1L << 11)
#define MSTATUS_MPP_U (0L << 11)
#define MSTATUS_MIE (1L << 3)  // machine-mode interrupt enable.

#define MIE_MEIE (1L << 11)  // external
#define MIE_MTIE (1L << 7)  // timer
#define MIE_MSIE (1L << 3)  // software

#define SSTATUS_SPP (1L << 8)  // Previous mode, 1=Supervisor, 0=User
#define SSTATUS_SPIE (1L << 5) // Supervisor Previous Interrupt Enable
#define SSTATUS_UPIE (1L << 4) // User Previous Interrupt Enable
#define SSTATUS_SIE (1L << 1)  // Supervisor Interrupt Enable
#define SSTATUS_UIE (1L << 0)  // User Interrupt Enable

#define SIE_SEIE (1L << 9)  // external
#define SIE_STIE (1L << 5)  // timer
#define SIE_SSIE (1L << 1)  // software


// which hart (core) is this?
static inline uint64_t r_mhartid()
{
  uint64_t x;
  asm volatile("csrr %0, mhartid" : "=r" (x) );
  return x;
}

static inline uint64_t r_mstatus()
{
  uint64_t x;
  asm volatile("csrr %0, mstatus" : "=r" (x) );
  return x;
}

static inline void w_mstatus(uint64_t x)
{
  asm volatile("csrw mstatus, %0" : : "r" (x));
}

// machine exception program counter, holds the
// instruction address to which a return from
// exception will go.
static inline void w_mepc(uint64_t x)
{
  asm volatile("csrw mepc, %0" : : "r" (x));
}

static inline uint64_t r_sstatus()
{
  uint64_t x;
  asm volatile("csrr %0, sstatus" : "=r" (x) );
  return x;
}

static inline void w_sstatus(uint64_t x)
{
  asm volatile("csrw sstatus, %0" : : "r" (x));
}

// Supervisor Interrupt Pending
static inline uint64_t r_sip()
{
  uint64_t x;
  asm volatile("csrr %0, sip" : "=r" (x) );
  return x;
}

static inline void w_sip(uint64_t x)
{
  asm volatile("csrw sip, %0" : : "r" (x));
}

static inline uint64_t r_sie()
{
  uint64_t x;
  asm volatile("csrr %0, sie" : "=r" (x) );
  return x;
}

static inline void w_sie(uint64_t x)
{
  asm volatile("csrw sie, %0" : : "r" (x));
}

static inline uint64_t r_mie()
{
  uint64_t x;
  asm volatile("csrr %0, mie" : "=r" (x) );
  return x;
}

static inline void w_mie(uint64_t x)
{
  asm volatile("csrw mie, %0" : : "r" (x));
}

// machine exception program counter, holds the
// instruction address to which a return from
// exception will go.
static inline void w_sepc(uint64_t x)
{
  asm volatile("csrw sepc, %0" : : "r" (x));
}

static inline uint64_t r_sepc()
{
  uint64_t x;
  asm volatile("csrr %0, sepc" : "=r" (x) );
  return x;
}

// Machine Exception Delegation
static inline uint64_t r_medeleg()
{
  uint64_t x;
  asm volatile("csrr %0, medeleg" : "=r" (x) );
  return x;
}

static inline void w_medeleg(uint64_t x)
{
  asm volatile("csrw medeleg, %0" : : "r" (x));
}

// Machine Interrupt Delegation
static inline uint64_t r_mideleg()
{
  uint64_t x;
  asm volatile("csrr %0, mideleg" : "=r" (x) );
  return x;
}

static inline void w_mideleg(uint64_t x)
{
  asm volatile("csrw mideleg, %0" : : "r" (x));
}

// Supervisor Trap-Vector Base Address
// low two bits are mode.
static inline void w_stvec(uint64_t x)
{
  asm volatile("csrw stvec, %0" : : "r" (x));
}

static inline uint64_t r_stvec()
{
  uint64_t x;
  asm volatile("csrr %0, stvec" : "=r" (x) );
  return x;
}

// Machine-mode interrupt vector
static inline void w_mtvec(uint64_t x)
{
  asm volatile("csrw mtvec, %0" : : "r" (x));
}

// supervisor address translation and protection;
// holds the address of the page table.
static inline void w_satp(uint64_t x)
{
  asm volatile("csrw satp, %0" : : "r" (x));
}

static inline uint64_t r_satp()
{
  uint64_t x;
  asm volatile("csrr %0, satp" : "=r" (x) );
  return x;
}

// Supervisor Scratch register, for early trap handler in trampoline.S.
static inline void w_sscratch(uint64_t x)
{
  asm volatile("csrw sscratch, %0" : : "r" (x));
}

static inline void w_mscratch(uint64_t x)
{
  asm volatile("csrw mscratch, %0" : : "r" (x));
}

// Supervisor Trap Cause
static inline uint64_t r_scause()
{
  uint64_t x;
  asm volatile("csrr %0, scause" : "=r" (x) );
  return x;
}

// Supervisor Trap Value
static inline uint64_t r_stval()
{
  uint64_t x;
  asm volatile("csrr %0, stval" : "=r" (x) );
  return x;
}

// Machine-mode Counter-Enable
static inline void w_mcounteren(uint64_t x)
{
  asm volatile("csrw mcounteren, %0" : : "r" (x));
}

static inline uint64_t r_mcounteren()
{
  uint64_t x;
  asm volatile("csrr %0, mcounteren" : "=r" (x) );
  return x;
}

// machine-mode cycle counter
static inline uint64_t r_time()
{
  uint64_t x;
  asm volatile("csrr %0, time" : "=r" (x) );
  return x;
}

// enable device interrupts
static inline void intr_on()
{
  w_sstatus(r_sstatus() | SSTATUS_SIE);
}

// disable device interrupts
static inline void intr_off()
{
  w_sstatus(r_sstatus() & ~SSTATUS_SIE);
}

// are device interrupts enabled?
static inline int intr_get()
{
  uint64_t x = r_sstatus();
  return (x & SSTATUS_SIE) != 0;
}

static inline uint64_t r_sp()
{
  uint64_t x;
  asm volatile("mv %0, sp" : "=r" (x) );
  return x;
}

// read and write tp, the thread pointer, which holds
// this core's hartid (core number), the index into cpus[].
static inline uint64_t r_tp()
{
  uint64_t x;
  asm volatile("mv %0, tp" : "=r" (x) );
  return x;
}

static inline void w_tp(uint64_t x)
{
  asm volatile("mv tp, %0" : : "r" (x));
}

static inline uint64_t r_ra()
{
  uint64_t x;
  asm volatile("mv %0, ra" : "=r" (x) );
  return x;
}

// flush the TLB.
static inline void sfence_vma()
{
  // the zero, zero means flush all TLB entries.
  asm volatile("sfence.vma zero, zero");
}

#define RISCV_CLOCKS_PER_SECOND 1000000
#define TICK_INTERVAL (RISCV_CLOCKS_PER_SECOND / NAUT_CONFIG_HZ)

#endif
