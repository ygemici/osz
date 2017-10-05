/*
 * stdint.h
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
 * @brief ISO C99: 7.18 Integer types <stdint.h>
 */

#ifndef _STDINT_H
#define _STDINT_H   1

#ifndef _AS
/* Exact integral types.  */

/* Signed.  */
typedef signed char         int8_t;
typedef short int           int16_t;
typedef int                 int32_t;
typedef long int            int64_t;

/* Unsigned.  */
typedef unsigned char       uint8_t;
typedef unsigned short int  uint16_t;
typedef unsigned int        uint32_t;
typedef unsigned long int   uint64_t;
#endif

/* Limits of integral types.  */

/* Minimum of signed integral types.  */
#define INT8_MIN        (-128)
#define INT16_MIN       (-32767-1)
#define INT32_MIN       (-2147483647-1)
#define INT64_MIN       (-9223372036854775807L-1)
/* Maximum of signed integral types.  */
#define INT8_MAX        (127)
#define INT16_MAX       (32767)
#define INT32_MAX       (2147483647)
#define INT64_MAX       (9223372036854775807L)

/* Maximum of unsigned integral types.  */
#define UINT8_MAX       (255)
#define UINT16_MAX      (65535)
#define UINT32_MAX      (4294967295U)
#define UINT64_MAX      (18446744073709551615UL)

/* Limit of `size_t' type.  */
#define SIZE_MAX        UINT64_MAX

/* Limits of `wchar_t'.  */
#define WCHAR_MIN       (0)
#define WCHAR_MAX       UINT16_MAX

#endif /* stdint.h */
