#pragma once

#ifndef __NAUTILUS_ULIB__
#define __NAUTILUS_ULIB__

// This header file contains the full system interface for the Nautilus
// userspace interface. Note: it is not meant to mirror how POSIX works, as that
// would be difficult :). At most, you can print to the console, spawn a new
// program, and exit the current program.

// Write some data to the console
int console_write(const char *message);

// Spawning a userspace program in Nautilus requires the user pass in the path
// to the binary (absolute path) and the argument. Note there is not an array of
// argv. Every program takes one and only string as argument to simplify the
// interface
int spawn(const char *program, const char *argument);

// Exit the program
void exit(void);

#endif