/*
 * sys/types.h
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
 * @brief System types
 */

#ifndef	_SYS_TYPES_H
#define	_SYS_TYPES_H	1

#include <stdint.h>

typedef void *va_list;
typedef unsigned char uchar;
typedef unsigned int uint;
typedef uint8_t bool_t;
typedef uint64_t size_t;
typedef uint64_t loff_t;
typedef uint64_t ino_t;
typedef uint64_t dev_t;
typedef uint64_t gid_t;
typedef uint64_t mode_t;
typedef uint32_t nlink_t;
typedef uint64_t uid_t;
typedef uint64_t off_t;
typedef uint64_t pid_t;
typedef uint64_t id_t;
typedef uint64_t ssize_t;
typedef uint64_t key_t;
typedef int register_t;
typedef uint64_t blksize_t;
typedef uint64_t blkcnt_t;	 /* Type to count number of disk blocks.  */
typedef uint64_t fsblkcnt_t; /* Type to count file system blocks.  */
typedef uint64_t fpos_t;

#endif /* sys/types.h */
