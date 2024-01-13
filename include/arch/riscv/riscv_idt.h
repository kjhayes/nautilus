#ifndef __RISCV_IDT_H__
#define __RISCV_IDT_H__

#include <nautilus/nautilus.h>

#define NUM_IDT_ENTRIES 256

int riscv_setup_idt(void);

int riscv_irq_install(ulong_t irq, int (*handler_addr)(ulong_t));
int riscv_idt_assign_entry(ulong_t entry, ulong_t handler_addr);
int riscv_idt_get_entry(ulong_t entry, ulong_t *handler_addr);

int riscv_null_irq_handler(ulong_t irq);
int riscv_handle_irq(ulong_t irq);


#endif
