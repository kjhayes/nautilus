/*
 * This file is part of the Nautilus AeroKernel developed
 * by the Constellation, Interweaving, Hobbes, and V3VEE
 * Projects with funding from the United States National
 * Science Foundation and the Department of Energy.
 *
 * The V3VEE Project is a joint project between Northwestern University
 * and the University of New Mexico.  The Hobbes Project is a collaboration
 * led by Sandia National Laboratories that includes several national
 * laboratories and universities. The Interweaving Project is a
 * joint project between Northwestern University and Illinois Institute
 * of Technology.   The Constellation Project is a joint project
 * between Northwestern University, Carnegie Mellon University,
 * and Illinois Institute of Technology.
 * You can find out more at:
 * http://www.v3vee.org
 * http://xstack.sandia.gov/hobbes
 * http://interweaving.org
 * http://constellation-project.net
 *
 * Copyright (c) 2023, Kevin Hayes <kjhayes@u.northwestern.edu>
 * Copyright (c) 2023, The V3VEE Project  <http://www.v3vee.org>
 *                     The Hobbes Project <http://xstack.sandia.gov/hobbes>
 * All rights reserved.
 *
 * Authors: Kevin Hayes <kjhayes@u.northwestern.edu>
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "LICENSE.txt".
 */

#ifndef __ENDIAN_H__
#define __ENDIAN_H__

#include<nautilus/arch.h>

#ifdef NAUT_CONFIG_ARCH_RISCV
inline static uint16_t bswap16(uint16_t n) {
  return ((n & 0xFF) << 8) | ((n >> 8) & 0xFF);
}
inline static uint32_t bswap32(uint32_t n) {
  return ((uint32_t)bswap16(n & 0xFFFF) << 16) | ((uint32_t)bswap16(n >> 16) & 0xFFFF);
}
inline static uint64_t bswap64(uint64_t n) {
  return ((uint64_t)bswap32(n & 0xFFFFFFFF) << 32) | ((uint64_t)bswap32(n >> 32) & 0xFFFFFFFF);
}
#else
#define bswap16(n) __builtin_bswap16(n)
#define bswap32(n) __builtin_bswap32(n)
#define bswap64(n) __builtin_bswap64(n)
#endif

// Sketchy function
static inline int check_little_endian(void) {
  volatile uint8_t arr[4] = {};
  for(int i = 0; i < 4; i++) {
    arr[i] = 0;
  }
  arr[0] = 1;

  volatile uint32_t val = *(uint32_t*)arr;
  if(val == 1) {
    return 1;
  } else {
    return 0;
  }
}

static inline uint16_t htobe16(uint16_t host) {
  return !arch_little_endian() ? host : bswap16(host);
}
static inline uint16_t htole16(uint16_t host) {
  return arch_little_endian() ? host : bswap16(host);
}
static inline uint16_t be16toh(uint16_t be) {
  return !arch_little_endian() ? be : bswap16(be);
}
static inline uint16_t le16toh(uint16_t le) {
  return arch_little_endian() ? le : bswap16(le);
}

static inline uint32_t htobe32(uint32_t host) {
  return !arch_little_endian() ? host : bswap32(host);
}
static inline uint32_t htole32(uint32_t host) {
  return arch_little_endian() ? host : bswap32(host);
}
static inline uint32_t be32toh(uint32_t be) {
  return !arch_little_endian() ? be : bswap32(be);
}
static inline uint32_t le32toh(uint32_t le) {
  return arch_little_endian() ? le : bswap32(le);
}

static inline uint64_t htobe64(uint64_t host) {
  return !arch_little_endian() ? host : bswap64(host);
}
static inline uint64_t htole64(uint64_t host) {
  return arch_little_endian() ? host : bswap64(host);
}
static inline uint64_t be64toh(uint64_t be) {
  return !arch_little_endian() ? be : bswap64(be);
}
static inline uint64_t le64toh(uint64_t le) {
  return arch_little_endian() ? le : bswap64(le);
}

#endif
