#ifndef __INIT_H__
#define __INIT_H__


void boot_stack_init(unsigned long hartid, unsigned long fdt);
void threaded_init(void);

#endif
