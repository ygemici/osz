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

#define public __attribute__ ((__visibility__("default")))
#define private __attribute__ ((__visibility__("hidden")))
#define c_assert(c) extern char cassert[(c)?0:-1]

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

typedef struct {
	uint32_t Data1;
	uint16_t Data2;
	uint16_t Data3;
	uint8_t Data4[8];
} __attribute__((packed)) uuid_t;
#define UUID_ACCESS(a) (a.Data4[7])

// type returned by syscalls clcall() and clrecv()
typedef struct {
    uint64_t evt;
    uint64_t arg0;
    uint64_t arg1;
    uint64_t arg2;
    uint64_t arg3;
    uint64_t arg4;
    uint64_t arg5;
    uint64_t ts;
} __attribute__((packed)) msg_t;
// bits in evt: 63TTT..TTTPFFFFFFFFFFFFFFF0
//  where T is a thread id or subsystem id, P true if message has a pointer,
//  F is a function number from 1 to 32767. Function number 0 is reserved.
#define MSG_SENDER(m) ((pid_t)((m)>>16))
#define MSG_FUNC(m) ((uint16_t)((m)&0x7FFF))
#define MSG_REGDATA (0)
#define MSG_PTRDATA (0x8000)
#define MSG_PTR(m) (m.arg0)
#define MSG_SIZE(m) (m.arg1)
#define MSG_MAGIC(m) (m.arg2)
#define MSG_ISREG(m) (!((m)&MSG_PTRDATA))
#define MSG_ISPTR(m) ((m)&MSG_PTRDATA)
#define MSG_DEST(t) ((uint64_t)(t)<<16)
//eg.:
//pid_t child = fork();
//msg->evt = MSG_DEST(child) | MSG_FUNC(anynumber) | MSG_PTRDATA

typedef struct {
    uint64_t quantum;
    uint64_t ticks[2];
    uint64_t datetime;    // in BCD yyyymmddhhiiss
    int64_t  timezone;    //in minutes -1440..1440
    uint64_t fb_width;
    uint64_t fb_height;
    uint64_t fb_scanline;
    uint64_t debug;
    uint64_t display;
    uint64_t fps;
    uint64_t rescueshell;
    uint64_t srand[4];
} __attribute__((packed)) sysinfo_t;
#endif /* sys/types.h */
