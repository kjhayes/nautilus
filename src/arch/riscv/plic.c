#include <arch/riscv/plic.h>
#include <arch/riscv/sbi.h>
#include <nautilus/cpu.h>
#include <nautilus/naut_types.h>
#include <nautilus/percpu.h>

typedef struct plic_context {
  off_t enable_offset;
  off_t context_offset;
} plic_context_t;

static addr_t plic_addr = 0;

static plic_context_t *contexts = NULL;

#define PLIC plic_addr
#define MREG(x) *((uint32_t *)(PLIC + (x)))

#define PLIC_PRIORITY_BASE 0x000000U

#define PLIC_PENDING_BASE 0x1000U

#define PLIC_ENABLE_BASE 0x002000U
#define PLIC_ENABLE_STRIDE 0x80U
#define IRQ_ENABLE 1
#define IRQ_DISABLE 0

#define PLIC_CONTEXT_BASE 0x200000U
#define PLIC_CONTEXT_STRIDE 0x1000U
#define PLIC_CONTEXT_THRESHOLD 0x0U
#define PLIC_CONTEXT_CLAIM 0x4U

#define PLIC_PRIORITY(n) (PLIC_PRIORITY_BASE + (n) * sizeof(uint32_t))
#define PLIC_ENABLE(n, h) (contexts[h].enable_offset + ((n) / 32) * sizeof(uint32_t))
#define PLIC_THRESHOLD(h) (contexts[h].context_offset + PLIC_CONTEXT_THRESHOLD)
#define PLIC_CLAIM(h) (contexts[h].context_offset + PLIC_CONTEXT_CLAIM)

// #define PLIC_PRIORITY MREG(PLIC + 0x0)
// #define PLIC_PENDING MREG(PLIC + 0x1000)
// #define PLIC_MENABLE(hart) MREG(PLIC + 0x2000 + (hart)*0x100)
// #define PLIC_SENABLE(hart) MREG(PLIC + 0x2080 + (hart)*0x100)
// #define PLIC_MPRIORITY(hart) MREG(PLIC + 0x201000 + (hart)*0x2000)
// #define PLIC_SPRIORITY(hart) MREG(PLIC + 0x200000 + (hart)*0x2000)
// #define PLIC_MCLAIM(hart) MREG(PLIC + 0x201004 + (hart)*0x2000)
// #define PLIC_SCLAIM(hart) MREG(PLIC + 0x200004 + (hart)*0x2000)

// #define ENABLE_BASE 0x2000
// #define ENABLE_PER_HART 0x100

void plic_init(void) {
    PLIC = 0x0c000000L;
}

static void plic_toggle(int hart, int hwirq, int priority, bool_t enable) {
    // printk("toggling on hart %d, irq=%d, priority=%d, enable=%d, plic=%p\n", hart, hwirq, priority, enable, PLIC);
    // off_t enable_base = PLIC + ENABLE_BASE + hart * ENABLE_PER_HART;
    // printk("enable_base=%p\n", enable_base);
    // uint32_t* reg = &(MREG(enable_base + (hwirq / 32) * 4));
    // printk("reg=%p\n", reg);
    // uint32_t hwirq_mask = 1 << (hwirq % 32);
    // printk("hwirq_mask=%p\n", hwirq_mask);
    // MREG(PLIC + 4 * hwirq) = 7;
    // PLIC_SPRIORITY(hart) = 0;

    // if (enable) {
    // *reg = *reg | hwirq_mask;
    // printk("*reg=%p\n", *reg);
    // } else {
    // *reg = *reg & ~hwirq_mask;
    // }

    // printk("*reg=%p\n", *reg);
}

void plic_enable(int hwirq, int priority)
{
    // plic_toggle(1, hwirq, priority, true);
}
void plic_disable(int hwirq)
{
    // plic_toggle(1, hwirq, 0, false);
}
int plic_claim(void)
{
    // return PLIC_SCLAIM(1);
    return 0;
}
void plic_complete(int irq)
{
    // PLIC_SCLAIM(1) = irq;
}
int plic_pending(void)
{
    return MREG(PLIC_PENDING_BASE);
}

void plic_dump(void)
{
    
}

void plic_init_hart(int hart) {
    // PLIC_SPRIORITY(hart) = 0;
    MREG(PLIC_PRIORITY(hart)) = 0;
    // printk("CPU ID: %d\n", my_cpu_id());
}

