#pragma once

#ifndef __NAUTILUS_ULIB__
#define __NAUTILUS_ULIB__


// This header file contains the full system interface for the Nautilus
// userspace interface. Note: it is not meant to mirror how POSIX works, as that
// would be difficult :). At most, you can print to the console, spawn a new
// program, and exit the current program.

// Write some data to the console
extern int console_write(const char *message);

// Spawning a userspace program in Nautilus requires the user pass in the path
// to the binary (absolute path) and the argument. Note there is not an array of
// argv. Every program takes one and only string as argument to simplify the
// interface
extern int spawn(const char *program, const char *argument);

// Exit the program
extern void exit(void);


// make a systemcall to the kernel. It is recommended you use the `syscall()` macro instead.
extern unsigned long __syscall(unsigned long nr, unsigned long a,
                               unsigned long b, unsigned long c,
                               unsigned long d);

#define __do_syscall(nr, a, b, c, d, ...) __syscall(nr, a, b, c, d)
#define syscall(nr, ...) __do_syscall((nr), __VA_ARGS__, 0, 0, 0, 0)

#endif