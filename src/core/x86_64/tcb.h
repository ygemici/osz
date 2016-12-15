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

/* WARNING: must match OSZ_tcb.evtq_ptr - OSZ_tcb.magic */
#define OSZ_tcb_evtq_ptr 16
#define OSZ_tcb_evtq_endptr 24
#define OSZ_tcb_gpr 48
#define OSZ_tcb_fx 176

/* WARNING: must match sizeof(OSZ_event) */
#define OSZ_event_size 32

#ifndef _AS
#include "../tcb.h"

#define OSZ_tcb_flag_needsxsave     4
#define OSZ_tcb_flag_needxmmsave    8

#define OSZ_event_sender(msg) ((pid_t)(t&(~(uint64_t)(__PAGESIZE-1))))
#define OSZ_event_function(msg) ((uint16_t)(t&((uint64_t)(__PAGESIZE-1))))

typedef struct {
    uint64_t msg;
    uint64_t arg0;
    uint64_t arg1;
    uint64_t arg2;
} __attribute__((packed)) OSZ_event;

typedef struct {
    uint32_t magic;
    uint8_t state;      // thread state
    uint8_t priority;   // thread priority
    uint16_t cpu;       // APIC ID of cpu on which this thread runs
    uint32_t flags;     // thread flags
    uint32_t evtq_cnt;  // event counter (number of items in event queue)
    OSZ_event *evtq_ptr;// event queue
    OSZ_event *evtq_endptr;
    uint64_t evtq_nextserial;
    uint64_t evtq_lastserial;
    uint64_t gpr[16];   // general purpose registers save area
    uint8_t fx[512];    // floating point registers save area (16 aligned)
    uint64_t corersp;   // core stack pointer
    uint64_t userrip;   // user instruction pointer on syscall
    uint64_t memroot;   // memory mapping root
    uint64_t userflags; // user space cpu flags
    uint64_t usererrno; // thread-safe libc errno
    uint64_t linkmem;   // number of linked memory pages
    uint64_t allocmem;  // number of allocated memory pages
    pid_t next;         // next thread in this priority level
    pid_t prev;         // previous thread in this priority level
    pid_t recvfrom;
    pid_t sendto;
    pid_t blksend;      // head of blocked threads chain (sending to this thread)
    uint64_t billcnt;   // ticks thread spent in userspace
    uint64_t syscnt;    // ticks thread spent in kernelspace (core)
    uint64_t self;      // self pointer (physical address of TLB PT)
    uid_t acl[128];     // access control list
} __attribute__((packed)) OSZ_tcb;

#include "ccb.h"        // CPU Control Block

#endif
