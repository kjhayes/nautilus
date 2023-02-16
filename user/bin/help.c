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
 * Copyright (c) 2023, Nick Wanninger <ncw@u.northwestern.edu>
 * Copyright (c) 2015, The V3VEE Project  <http://www.v3vee.org>
 *                     The Hobbes Project <http://xstack.sandia.gov/hobbes>
 * All rights reserved.
 *
 * Author: Nick Wanninger <ncw@u.northwestern.edu>
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "LICENSE.txt".
 */

#include <ulib.h>

// This is a simple program which prints help messages

int main() {
  // clang-format off
  printf("Welcome to the Nautilus userspace!\n");
  printf("You are currently in a very simple shell, and there are a few commands available to you. ");
  printf("To exit the shell, type `exit`. ");
  printf("The `hello` program is the simplest program there is, and just prints Hello world to the console. ");
  printf("In a real system, the `init` program would be responsible for starting up all the userspace services such as the desktop environment, any networking daemons, or whatever your userspace env needs. ");
  printf("Here, `init` is very simple and only runs `sh`. ");
  printf("The `hack` program tests some security features of the kernel. Mainly, it focuses on checking if pages are zeroed when they are allocated to the process. ");
  printf("This userspace is *NOT* secure, and has many vulnerabilities. If you want to prove this to yourself, write a program and pass a kernel address into the second argument of the `write` system call. ");
  printf("(the kernel blindly accepts this pointer and writes the contents of the file argument to it). ");
  // clang-format on

  // printf("The `hello` program is the simplest program there is, and just prints Hello world to the console. ");
  printf("\n");
  return 0;
}
