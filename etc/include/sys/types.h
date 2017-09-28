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
 * @brief OS/Z specific system types
 */

#ifndef	_SYS_TYPES_H
#define	_SYS_TYPES_H	1

#include <stdint.h>

/*** visibility ***/
#define public __attribute__ ((__visibility__("default")))
#define private __attribute__ ((__visibility__("hidden")))

/*** common defines ***/
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

/*** Access bits in uuid.Data4[7] ***/
#define A_READ    (1<<0)
#define A_WRITE   (1<<1)
#define A_EXEC    (1<<2)          // execute or search
#define A_APPEND  (1<<3)
#define A_DELETE  (1<<4)
#define A_SUID    (1<<6)          // Set user id on execution
#define A_SGID    (1<<7)          // Inherit ACL, no set group per se in OS/Z

/*** libc ***/
#ifndef _AS
#include <stdint.h>

typedef struct {
	uint32_t Data1;
	uint16_t Data2;
	uint16_t Data3;
	uint8_t Data4[8];
} __attribute__((packed)) uuid_t;
#define UUID_ACCESS(a) (a.Data4[7])

typedef unsigned char uchar;
typedef unsigned int uint;
typedef uint8_t bool_t;
typedef uint64_t size_t;
typedef uint64_t fid_t;
typedef uint64_t ino_t;
typedef uint64_t dev_t;
typedef uuid_t gid_t;
typedef uint64_t mode_t;
typedef uint32_t nlink_t;
typedef uuid_t uid_t;
typedef int64_t off_t;
typedef uint64_t pid_t;
typedef uint64_t id_t;
typedef uint64_t ssize_t;
typedef uint64_t key_t;
typedef int register_t;
typedef uint64_t blksize_t;
typedef uint64_t blkcnt_t;	 /* Type to count number of disk blocks.  */
typedef uint64_t fsblkcnt_t; /* Type to count file system blocks.  */
typedef uint64_t fpos_t;
typedef uint32_t keymap_t;
/*
typedef void __signalfn(int);
typedef __signalfn *sighandler_t;
*/
typedef void (*sighandler_t) (int);

#define c_assert(c) extern char cassert[(c)?0:-1]

typedef uint64_t evt_t;
typedef uint64_t phy_t;
typedef uint64_t virt_t;

// type returned by syscalls mq_call() and mq_recv()
typedef struct {
    evt_t evt;     //MSG_DEST(pid) | MSG_FUNC(funcnumber) | MSG_PTRDATA
    uint64_t arg0; //Buffer address if MSG_PTRDATA, otherwise undefined
    uint64_t arg1; //Buffer size if MSG_PTRDATA
    uint64_t arg2; //Buffer type if MSG_PTRDATA
    uint64_t arg3;
    uint64_t arg4;
    uint64_t arg5;
    uint64_t serial;
} __attribute__((packed)) msg_t;
// bits in evt: (63)TTT..TTT P FFFFFFFFFFFFFFF(0)
//  where T is a task id or subsystem id, P true if message has a pointer,
//  F is a function number from 1 to 32766. Function number 0 and 32767 are reserved.
#define EVT_DEST(t) ((uint64_t)(t)<<16)
#define EVT_SENDER(m) ((pid_t)((m)>>16))
#define EVT_FUNC(m) ((uint16_t)((m)&0x7FFF))
#define MSG_REGDATA (0)
#define MSG_PTRDATA (0x8000)
#define MSG_PTR(m) (m.arg0)
#define MSG_SIZE(m) (m.arg1)
#define MSG_MAGIC(m) (m.arg2)
#define MSG_ISREG(m) (!((m)&MSG_PTRDATA))
#define MSG_ISPTR(m) ((m)&MSG_PTRDATA)
#endif

#endif /* sys/types.h */
