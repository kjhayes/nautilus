#pragma once

#ifndef __NAUTILUS_KERNEL_UAPI__
#define __NAUTILUS_KERNEL_UAPI__

#define SYSCALL_EXIT 0
#define SYSCALL_WRITE 1
#define SYSCALL_GETC 2
#define SYSCALL_SPAWN 3
#define SYSCALL_WAIT 4
#define SYSCALL_VALLOC 5 // get some new virtual memory from the kernel (in pages, not bytes)
#define SYSCALL_VFREE 6 // return some region of virtual memory to the kernel (TODO)'
#define SYSCALL_YIELD 7 // yield the thread

#endif