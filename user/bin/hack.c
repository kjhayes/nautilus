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

// The main purpose of this program is to show how you must be careful when
// accessing pointers in the kernel that the user gave to you. It also tests
// that memory allocated by the kernel is zeroed. If it isn't we could be
// leaking data from other programs! In a real kernel environment, that data
// might be passwords or encryption keys!

#include <ulib.h>

void hexdump(void *vbuf, long len) {
  unsigned char *buf = vbuf;

  int w = 16;
  for (int i = 0; i < len; i += w) {
    unsigned char *line = buf + i;
    // printf("%p: ", (void *)(long)i);
    for (int c = 0; c < w; c++) {
      if (i + c >= len) {
        printf("   ");
      } else {
        printf("%x%x ", (line[c] & 0xF0) >> 4, line[c] & 0x0F);
      }
    }
    printf(" |");
    for (int c = 0; c < w; c++) {
      if (i + c >= len) {
      } else {
        printf("%c", (line[c] < 0x20) || (line[c] > 0x7e) ? '.' : line[c]);
      }
    }
    printf("|\n");
  }
}

int main() {

  void *kernel_address = (void *)0x1000;
  printf("I'm about to print memory from the kernel.\n");
  // Nautilus doesn't protect against the user doing sneaky stuff like this!
  write(kernel_address, 0x1000);
  printf("\nWhy did that work?\n");




  printf("Now, I'm going to allocate some memory and\nprint it out. It better be all zeroes!\n");
  // allocate a page
  void *x = valloc(1);
  hexdump(x, 256); // hexdump some bytes
  memset(x, 0xFA, 4096); // Wipe it ourselves. Maybe nextime we'll get the same page?
  return 0;
}
