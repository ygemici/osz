/*
 * core/x86_64/tcb.h
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
 * @brief Thread Control Block, architecture specific part
 */

#include <limits.h>

/* WARNING: must match tcb struct, for asm */
#define OSZ_tcb_gpr   8
#define OSZ_tcb_fx  128
#define OSZ_tcb_mypid 640
#define OSZ_tcb_memroot 648

/* platform specific TCB flags */
#define OSZ_tcb_needsxsave     32

#ifndef _AS
#include "../tcb.h"

// must be __PAGESIZE long
typedef struct {
    uint32_t magic;
    uint8_t state;      // thread state and flags
    uint8_t priority;   // thread priority
    uint16_t cpu;       // APIC ID of cpu on which this thread runs
    // tcb+8
    uint8_t gpr[120];   // general purpose registers save area
    uint8_t fx[512];    // floating point and media registers save area (16 aligned)
    pid_t mypid;        // thread's pid at 640
    uint64_t memroot;   // memory mapping root at 648
    // ordering does not matter
    pid_t sendto;
    pid_t blksend;      // head of blocked threads chain (sending to this thread)
    pid_t next;         // next thread in this priority level
    pid_t prev;         // previous thread in this priority level
    uint64_t linkmem;   // number of linked memory pages
    uint64_t allocmem;  // number of allocated memory pages
    uint64_t billcnt;   // ticks thread spent in userspace
    uint64_t syscnt;    // ticks thread spent in kernelspace (core)
    uint64_t self;      // self pointer (physical address of TLB PT)
    uint64_t errno;     // thread safe libc errno
    uuid_t acl[128];     // access control list
    uint8_t padding[__PAGESIZE-2784-40];
    uint64_t rip;		// at the end of ist_usr stack an extra 40 bytes
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} __attribute__((packed)) OSZ_tcb;

#endif
