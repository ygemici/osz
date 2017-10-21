/*
 * limits.h
 *
 * Copyright 2016 CC-by-nc-sa bztsrc@github
 * https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * You are free to:
 *
 * - Share — copy and redistribute the material in any medium or format
 * - Adapt — remix, transform, and build upon the material
 *     The licensor cannot revoke these freedoms as long as you follow
 *     the license terms.
 *
 * Under the following terms:
 *
 * - Attribution — You must give appropriate credit, provide a link to
 *     the license, and indicate if changes were made. You may do so in
 *     any reasonable manner, but not in any way that suggests the
 *     licensor endorses you or your use.
 * - NonCommercial — You may not use the material for commercial purposes.
 * - ShareAlike — If you remix, transform, or build upon the material,
 *     you must distribute your contributions under the same license as
 *     the original.
 *
 * @brief System limits and sizes of types
 */

/*
 *	ISO C99 Standard: 7.10/5.2.4.2.1 Sizes of integer types	<limits.h>
 */

#ifndef _LIMITS_H_
#define _LIMITS_H   1

#define TMP_MAX         8
#define FILENAME_MAX    111

#define __WORDSIZE  64
#define __PAGESIZE  4096
#define __SLOTSIZE  (4096*512)
#define __BUFFSIZE  (__SLOTSIZE/2)

// memory limits, OS/Z specific addresses
#define MQ_ADDRESS     (__PAGESIZE)
#define TEXT_ADDRESS   (__SLOTSIZE)                // 2M code segment
#define BSS_ADDRESS    (0x0000000100000000UL)      // 4G data, tls memory, see bztalloc.c
#define SBSS_ADDRESS   (0xFFFFFF8000000000UL)      // -512G shared memory, see bztalloc.c

#define CHAR_BIT    8
#define SCHAR_MIN   (-128)
#define SCHAR_MAX   127
#define UCHAR_MAX   255
#define CHAR_MIN    SCHAR_MIN
#define CHAR_MAX    SCHAR_MAX
#define SHRT_MIN    (-32768)
#define SHRT_MAX    32767
#define USHRT_MAX   65535
#define INT_MIN     (-INT_MAX - 1)
#define INT_MAX     2147483647
#define UINT_MAX    4294967295U
#define LONG_MAX    9223372036854775807L
#define LONG_MIN    (-LONG_MAX - 1L)
#define ULONG_MAX   18446744073709551615UL
#define LLONG_MAX   9223372036854775807LL
#define LLONG_MIN   (-LLONG_MAX - 1LL)
#define ULLONG_MAX  18446744073709551615ULL

/* The largest number rand will return.  */
#define	RAND_MAX    ULLONG_MAX

#endif	/* limits.h */
