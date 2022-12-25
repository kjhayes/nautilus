#pragma once

#ifndef __NAUTILUS_ULIB__
#define __NAUTILUS_ULIB__

#include "../../include/nautilus/uapi.h"

// This header file contains the full system interface for the Nautilus
// userspace interface. Note: it is not meant to mirror how POSIX works, as that
// would be difficult :). At most, you can print to the console, spawn a new
// program, and exit the current program.

// first, some typedefs
typedef unsigned int uint32_t;
typedef unsigned long uint64_t;

typedef int int32_t;
typedef long int64_t;

// Write some data to the console
extern int console_write(const char *message);

// Spawning a userspace program in Nautilus requires the user pass in the path
// to the binary (absolute path) and the argument. Note there is not an array of
// argv. Every program takes one and only string as argument to simplify the
// interface
extern int spawn(const char *program, const char *argument);

// Exit the program
extern void exit(void);

extern long strlen(const char *s);
extern void printf(char *fmt, ...);
extern void putc(char c);

// These macros declare interfaces to make systemcalls to the kernel. There are
// 4 of them to allow you to invoke a systemcall `nr` up to three arguments,
// `a,b,c`.
#define syscall0(nr)                                                           \
  ({                                                                           \
    unsigned long ret;                                                         \
    asm volatile("int $0x80" : "=a"(ret) : "0"(nr) : "memory");                \
    ret;                                                                       \
  });

#define syscall1(nr, a)                                                        \
  ({                                                                           \
    unsigned long ret;                                                         \
    asm volatile("int $0x80" : "=a"(ret) : "0"(nr), "D"(a) : "memory");        \
    ret;                                                                       \
  });

#define syscall2(nr, a, b)                                                     \
  ({                                                                           \
    unsigned long ret;                                                         \
    asm volatile("int $0x80"                                                   \
                 : "=a"(ret)                                                   \
                 : "0"(nr), "D"(a), "S"(b)                                     \
                 : "memory");                                                  \
    ret;                                                                       \
  });

#define syscall3(nr, a, b, c)                                                  \
  ({                                                                           \
    unsigned long ret;                                                         \
    asm volatile("int $0x80"                                                   \
                 : "=a"(ret)                                                   \
                 : "0"(nr), "D"(a), "S"(b), "d"(c)                             \
                 : "memory");                                                  \
    ret;                                                                       \
  });

#endif