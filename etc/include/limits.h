/* Copyright (C) 1991-2016 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

/*
 *	ISO C99 Standard: 7.10/5.2.4.2.1 Sizes of integer types	<limits.h>
 */

#ifndef _LIBC_LIMITS_H_
#define _LIBC_LIMITS_H_	1
#define _LIMITS_H	1

/* Maximum length of any multibyte character in any locale.
   We define this value here since the gcc header does not define
   the correct value.  */
#define MB_LEN_MAX	16

#define TMP_MAX	8
#define FILENAME_MAX	111


#define __WORDSIZE	64
#define __PAGESIZE	4096
#define __SYSCALL_WORDSIZE		64

// some OS/Z specific values for addresses
#define TCB_ADDRESS (0)
#define MQ_ADDRESS (__PAGESIZE)
#define TEXT_ADDRESS (0x200000)
#define BSS_ADDRESS (0x100000000)
// (TEXT_ADDRESS-MQ_ADDRESS-stacksizemax)/__PAGESIZE
#define NRMQ_MAX	63

#  define CHAR_BIT	8
#  define SCHAR_MIN	(-128)
#  define SCHAR_MAX	127
#  define UCHAR_MAX	255
#  ifdef __CHAR_UNSIGNED__
#   define CHAR_MIN	0
#   define CHAR_MAX	UCHAR_MAX
#  else
#   define CHAR_MIN	SCHAR_MIN
#   define CHAR_MAX	SCHAR_MAX
#  endif
#  define SHRT_MIN	(-32768)
#  define SHRT_MAX	32767
#  define USHRT_MAX	65535
#  define INT_MIN	(-INT_MAX - 1)
#  define INT_MAX	2147483647
#  define UINT_MAX	4294967295U
#  define LONG_MAX	9223372036854775807L
#  define LONG_MIN	(-LONG_MAX - 1L)
#  define ULONG_MAX	18446744073709551615UL
#  define LLONG_MAX	9223372036854775807LL
#  define LLONG_MIN	(-LLONG_MAX - 1LL)
#  define ULLONG_MAX	18446744073709551615ULL

#endif	/* !_LIBC_LIMITS_H_ */

 /* Get the compiler's limits.h, which defines almost all the ISO constants.

    We put this #include_next outside the double inclusion check because
    it should be possible to include this file more than once and still get
    the definitions from gcc's header.  */
#if defined __GNUC__ && !defined _GCC_LIMITS_H_
/* `_GCC_LIMITS_H_' is what GCC's file defines.  */
# include_next <limits.h>
#endif
