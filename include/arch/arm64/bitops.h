#ifndef _ASM_ARM64_BITOPS_H
#define _ASM_ARM64_BITOPS_H

/*
 * Copyright 1992, Linus Torvalds.
 *
 * Note: inlines with more than a single statement should be marked
 * __always_inline to avoid problems with older gcc's inlining heuristics.
 */

#ifndef __BITOPS_H__
#error only <lib/bitops.h> can be included directly
#endif

#include <nautilus/intrinsics.h>


#define BIT_64(n)	(U64_C(1) << (n))

/*
 * These have to be done with inline assembly: that way the bit-setting
 * is guaranteed to be atomic. All bit operations return 0 if the bit
 * was cleared before the operation and != 0 if it was not.
 *
 * bit 0 is the LSB of addr; bit 32 is the LSB of (addr+1).
 */

#if __GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 1)
/* Technically wrong, but this avoids compilation errors on some gcc
   versions. */
#define BITOP_ADDR(x) "=m" (*(volatile long *) (x))
#else
#define BITOP_ADDR(x) "+m" (*(volatile long *) (x))
#endif

#define ADDR				BITOP_ADDR(addr)

/*
 * We do the locked ops that don't return the old value as
 * a mask operation on a byte.
 */

#define IS_IMMEDIATE(nr)		(__builtin_constant_p(nr))
#define CONST_MASK_ADDR(nr, addr)	BITOP_ADDR((void *)(addr) + ((nr)>>3))
#define CONST_MASK(nr)			(1 << ((nr) & 7))

#define BIT_WORD(nr)		((nr) / BITS_PER_LONG)
#define BIT_ULL_WORD(nr)	((nr) / BITS_PER_LONG_LONG)
#define BITS_PER_BYTE		8

/* Bitmask modifiers */
#define __NOP(x)	(x)
#define __NOT(x)	(~(x))


/**
 * __set_bit - Set a bit in memory
 * @nr: the bit to set
 * @addr: the address to start counting from
 *
 * Unlike set_bit(), this function is non-atomic and may be reordered.
 * If it's called on the same region of memory simultaneously, the effect
 * may be that only one operation succeeds.
 */
static inline void set_bit(int nr, volatile unsigned long *addr)
{
    addr[BIT_WORD((uint64_t)nr)] |= (1UL<<((uint64_t)nr % BITS_PER_LONG));
}


static inline void clear_bit(int nr, volatile unsigned long *addr)
{
    addr[BIT_WORD((uint64_t)nr)] &= ~(1UL << ((uint64_t)nr % BITS_PER_LONG));
}

static inline int test_bit(unsigned int nr, const volatile unsigned long *addr)
{
    return 1UL & (addr[BIT_WORD((uint64_t)nr)] >> ((uint64_t)nr % BITS_PER_LONG));
}

/**
 * test_and_set_bit - Set a bit and return its old value
 * @nr: Bit to set
 * @addr: Address to count from
 *
 * This operation is atomic and cannot be reordered.
 * It also implies a memory barrier.
 */
static inline int test_and_set_bit(int nr, volatile unsigned long * addr)
{
    addr += BIT_WORD((uint64_t)nr);
    unsigned long bit = 1UL<<((uint64_t)nr % BITS_PER_LONG);

    unsigned long old;
    if(arch_atomics_enabled()) {
      old = __atomic_fetch_or(addr, bit, __ATOMIC_SEQ_CST);
    } else {
      old = *addr;
      *addr |= bit;
    }
    return old & bit;
}

/**
 * test_and_clear_bit - Clear a bit and return its old value
 * @nr: Bit to clear
 * @addr: Address to count from
 *
 * This operation is atomic and cannot be reordered.
 * It also implies a memory barrier.
 */
static inline int test_and_clear_bit(int nr, volatile unsigned long * addr)
{
  addr += BIT_WORD((uint64_t)nr);
  unsigned long bit = 1UL<<((uint64_t)nr % BITS_PER_LONG);
  
  unsigned long old; 
  if(arch_atomics_enabled()) {
    old = __atomic_fetch_and(addr, ~bit, __ATOMIC_SEQ_CST);
  } else {
    old = *addr;
    *addr &= ~bit;
  }
  return (old & bit);
}

/**
 * __change_bit - Toggle a bit in memory
 * @nr: the bit to change
 * @addr: the address to start counting from
 *
 * Unlike change_bit(), this function is non-atomic and may be reordered.
 * If it's called on the same region of memory simultaneously, the effect
 * may be that only one operation succeeds.
 */
static inline void change_bit(int nr, volatile unsigned long *addr)
{
    addr[BIT_WORD((uint64_t)nr)] ^= (1UL<<((uint64_t)nr % BITS_PER_LONG));
}


/**
 * __ffs - find first set bit in word
 * @word: The word to search
 *
 * Undefined if no bit exists, so code should check against 0 first.
 */
static inline unsigned long __ffs(unsigned long word)
{
    int num = 0;

    if ((word & 0xffffffff) == 0) {
        num += 32;
    	word >>= 32;
    }
    if ((word & 0xffff) == 0) {
    	num += 16;
    	word >>= 16;
    }
    if ((word & 0xff) == 0) {
    	num += 8;
    	word >>= 8;
    }
    if ((word & 0xf) == 0) {
    	num += 4;
    	word >>= 4;
    }
    if ((word & 0x3) == 0) {
    	num += 2;
    	word >>= 2;
    }
    if ((word & 0x1) == 0) {
    	num += 1;
    }
    return num;
}

/**
 * ffz - find first zero bit in word
 * @word: The word to search
 *
 * Undefined if no zero exists, so code should check against ~0UL first.
 */
#define ffz(x) __ffs(~(x))

/**
 * __fls - find last (most-significant) set bit in a long word
 * @word: the word to search
 *
 * Undefined if no set bit exists, so code should check against 0 first.
 */
static inline unsigned long __fls(unsigned long word)
{
	int num = BITS_PER_LONG - 1;

	if (!(word & (~0ul << 32))) {
		num -= 32;
		word <<= 32;
	}
	if (!(word & (~0ul << (BITS_PER_LONG-16)))) {
		num -= 16;
		word <<= 16;
	}
	if (!(word & (~0ul << (BITS_PER_LONG-8)))) {
		num -= 8;
		word <<= 8;
	}
	if (!(word & (~0ul << (BITS_PER_LONG-4)))) {
		num -= 4;
		word <<= 4;
	}
	if (!(word & (~0ul << (BITS_PER_LONG-2)))) {
		num -= 2;
		word <<= 2;
	}
	if (!(word & (~0ul << (BITS_PER_LONG-1))))
		num -= 1;
	return num;
}

/**
 * ffs - find first set bit in word
 * @x: the word to search
 *
 * This is defined the same way as the libc and compiler builtin ffs
 * routines, therefore differs in spirit from the other bitops.
 *
 * ffs(value) returns 0 if value is 0 or the position of the first
 * set bit if value is nonzero. The first (least significant) bit
 * is at position 1.
 */
static inline int ffs(int x)
{
    return __builtin_ffs(x);
}

/**
 * fls - find last (most-significant) bit set
 * @x: the word to search
 *
 * This is defined the same way as ffs.
 * Note fls(0) = 0, fls(1) = 1, fls(0x80000000) = 32.
 */
static inline int fls(unsigned int x)
{
	int r = 32;

	if (!x)
		return 0;
	if (!(x & 0xffff0000u)) {
		x <<= 16;
		r -= 16;
	}
	if (!(x & 0xff000000u)) {
		x <<= 8;
		r -= 8;
	}
	if (!(x & 0xf0000000u)) {
		x <<= 4;
		r -= 4;
	}
	if (!(x & 0xc0000000u)) {
		x <<= 2;
		r -= 2;
	}
	if (!(x & 0x80000000u)) {
		x <<= 1;
		r -= 1;
	}
	return r;
}

/**
 * fls64 - find last set bit in a 64-bit word
 * @x: the word to search
 *
 * This is defined in a similar way as the libc and compiler builtin
 * ffsll, but returns the position of the most significant set bit.
 *
 * fls64(value) returns 0 if value is 0 or the position of the last
 * set bit if value is nonzero. The last (most significant) bit is
 * at position 64.
 */
static inline int fls64(unsigned long x)
{
	if (x == 0)
		return 0;
	return __fls(x) + 1;
}

static inline int __ctzdi2(unsigned long x)
{
        return __ffs((unsigned int)x);
}

#endif /* _ASM_ARM64_BITOPS_H */
