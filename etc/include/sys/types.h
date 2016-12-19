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

#ifndef	_TYPES_H
#define	_TYPES_H	1

#include <stdint.h>

#ifndef NULL
#define NULL ((void *)0)
#endif

#ifndef null
#define null NULL
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef true
#define true 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef false
#define false 0
#endif

typedef void *va_list;
typedef unsigned char uchar;
typedef unsigned int uint;
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

typedef struct {
	uint32_t Data1;
	uint16_t Data2;
	uint16_t Data3;
	uint8_t Data4[8];
} __attribute__((packed)) uuid_t;
#define UUID_ACCESS(a) (a.Data4[7])

typedef struct {
    uint64_t evt;
    uint64_t arg0;
    uint64_t arg1;
    uint64_t arg2;
} __attribute__((packed)) msg_t;
#define MSG_SENDER(m) ((pid_t)(m.evt>>16))
#define MSG_TYPE(m) ((uint16_t)(m.evt&0xFFFF))
#define EVT_SENDER(e) ((pid_t)(e>>16))
#define EVT_TYPE(e) ((uint16_t)(e&0xFFFF))

#endif /* sys/types.h */
