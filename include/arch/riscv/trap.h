#ifndef __TRAP_H__
#define __TRAP_H__

void trap_init(void);

struct nk_irq_dev;
int riscv_set_root_irq_dev(struct nk_irq_dev *dev);

#endif
