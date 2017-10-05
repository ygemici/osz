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
#define A_EXEC    (1<<2)    // execute or search
#define A_APPEND  (1<<3)
#define A_DELETE  (1<<4)
#define A_SUID    (1<<6)    // Set user id on execution
#define A_SGID    (1<<7)    // Inherit ACL, no set group per se in OS/Z

/*** libc ***/
#ifndef _AS

typedef struct {
	uint32_t Data1;
	uint16_t Data2;
	uint16_t Data3;
	uint8_t Data4[8];
} __attribute__((packed)) uuid_t;
#define UUID_ACCESS(a) (a.Data4[7])

typedef struct
  {
    int quot;               // Quotient.
    int rem;                // Remainder.
  } div_t;

typedef struct
  {
    long int quot;          // Quotient.
    long int rem;           // Remainder.
  } ldiv_t;

typedef struct
  {
    long long int quot;     // Quotient.
    long long int rem;      // Remainder.
  } lldiv_t;

typedef unsigned char uchar;
typedef unsigned int uint;
typedef uint8_t bool_t;     // boolean type, true or false
typedef uint64_t size_t;    // buffer size
typedef uint64_t fid_t;     // file index
typedef uint64_t ino_t;     // inode number
typedef uint64_t dev_t;     // device index
typedef uuid_t gid_t;       // group id
typedef uint64_t mode_t;    // mode
typedef uint64_t nlink_t;   // number of links
typedef uuid_t uid_t;       // user id
typedef int64_t off_t;      // offset
typedef uint64_t pid_t;     // process id
typedef uint64_t id_t;      // identify
typedef uint64_t ssize_t;   // ?
typedef uint64_t key_t;     // ?
typedef int register_t;     // register type
typedef uint64_t blksize_t; // block size
typedef uint64_t blkcnt_t;  // number of disk blocks.
typedef uint64_t fpos_t;    // file position
typedef uint32_t keymap_t;  // keymap entry
typedef uint64_t time_t;    // timestamp in microsec

typedef struct {
    fid_t       st_dev;     // ID of device, *not* major/minor
    ino_t       st_ino;     // inode of the file
    mode_t      st_mode;    // mode
    uint8_t     st_type[4]; // file type
    uint8_t     st_mime[44];// mime type
    nlink_t     st_nlink;   // number of hard links
    uid_t       st_owner;   // owner user id
    off_t       st_size;    // file size
    blksize_t   st_blksize; // block size
    blkcnt_t    st_blocks;  // number of allocated blocks
    time_t      st_atime;   // access time in microsec timestamp
    time_t      st_mtime;   // modification time in microsec timestamp
    time_t      st_ctime;   // status change time in microsec timestamp
} stat_t;

typedef struct {
    fid_t       d_dev;      // ID of device, *not* major/minor
    ino_t       d_ino;      // inode number
    uint8_t     d_icon[8];  // short mime type for icon
    uint64_t    d_filesize; // file size
    uint32_t    d_type;     // entry type, st_mode >> 16
    uint32_t    d_len;      // name length
    char        d_name[FILENAME_MAX];
} dirent_t;

/*
typedef void __signalfn(int);
typedef __signalfn *sighandler_t;
*/
typedef void (*sighandler_t) (int);

#define c_assert(c) extern char cassert[(c)?0:-1]

typedef uint64_t phy_t;     // physical address
typedef uint64_t virt_t;    // virtual address
typedef uint64_t evt_t;     // event type

// type returned by syscalls mq_call() and mq_recv()
typedef struct {
    evt_t evt;     //MSG_DEST(pid) | MSG_FUNC(funcnumber) | MSG_PTRDATA
    union {
        // !MSG_PTRDATA, only scalar values
        struct {
            uint64_t arg0;
            uint64_t arg1;
            uint64_t arg2;
            uint64_t arg3;
            uint64_t arg4;
            uint64_t arg5;
        };
        // MSG_PTRDATA, buffer mapped along with message
        struct {
            void *ptr;      //Buffer address if MSG_PTRDATA, otherwise undefined
            uint64_t size;  //Buffer size if MSG_PTRDATA
            uint64_t type;  //Buffer type if MSG_PTRDATA
            uint64_t attr0;
            uint64_t attr1;
            uint64_t attr2;
        };
    };
    uint64_t serial;
} __attribute__((packed)) msg_t;
// bits in evt: (63)TTT..TTT P FFFFFFFFFFFFFFF(0)
//  where T is a task id or subsystem id, P true if message has a pointer,
//  F is a function number from 1 to 32766. Function numbers 0 and 32767 are reserved.
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
