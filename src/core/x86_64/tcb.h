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
#define tcb_errno 6
#define tcb_gpr   8
#define tcb_fx  128
#define tcb_mypid 640
#define tcb_memroot 648
#define tcb_excerr 680
#define tcb_sigmask 808
#define tcb_sigpending 812

/* platform specific TCB flags */
#define tcb_flag_needsxsave     128

#ifndef _AS
#include "../tcb.h"

// maximum size of IRQ handler routine's stack
#define TCB_ISRSTACK 512
// maximum number of ACE
#define TCB_MAXACE 128

/* please note that this structure is referenced a lot from asm
 * which can't use structs, so I've given the relative offsets for
 * each field in the struct. The current thread's tcb is at 0. */

// must be __PAGESIZE long
typedef struct {
    uint32_t magic;     // Thread Control Block magic, must be 'THRD'
    uint8_t priority;   // thread priority
    uint8_t state;      // thread state and flags
    uint16_t errno;     // thread safe libc errno            tcb+   6
    uint8_t gpr[120];   // general purpose registers area at tcb+   8
    uint8_t fx[512];    // media registers save area at      tcb+ 128
    pid_t mypid;        // thread's pid at                   tcb+ 640
    uint64_t memroot;   // memory mapping root at            tcb+ 648
    uint64_t billcnt;   // ticks thread spent in ring 3      tcb+ 656
    uint64_t syscnt;    // ticks thread spent in ring 0      tcb+ 664
    uint64_t cpu;       // APIC ID of executor cpu           tcb+ 672
    uint64_t excerr;    // exception error code              tcb+ 680
    pid_t sendto;       // destination thread                tcb+ 688
    pid_t blksend;      // head of threads waiting to send   tcb+ 696
    pid_t next;         // next thread in priority level     tcb+ 704
    pid_t prev;         // previous thread in priority level tcb+ 712
    pid_t alarm;        // next thread in alarm queue        tcb+ 720
    uint64_t alarmsec;  // when to wake up (UTC timestamp)   tcb+ 728
    uint64_t alarmns;   // when to wake up (nanosec)         tcb+ 736
    uint64_t linkmem;   // number of linked memory pages     tcb+ 744
    uint64_t allocmem;  // number of allocated memory pages  tcb+ 752
    uint64_t self;      // self pointer (phy_t of TLB PT)    tcb+ 760
    uint64_t blktime;   // time when thread was blocked      tcb+ 768
    uint64_t blkcnt;    // ticks thread spent blocked        tcb+ 776
    uint64_t swapminor; // swap inode when hybernated        tcb+ 784
    uint64_t cmdline;   // pointer to argc in stack          tcb+ 792
    uint64_t name;      // pointer to argv[0] in stack       tcb+ 800
    sighandler_t sighandler[32]; // for receiving signals    tcb+ 808
    uuid_t acl[TCB_MAXACE];// access control list            tcb+ 1064

    // compile time check for minimum stack size
    uint8_t padding[__PAGESIZE-TCB_MAXACE*16-TCB_ISRSTACK- 1064 ];
    uint8_t irqhandlerstack[TCB_ISRSTACK-40];

    // the last 40 bytes of stack is referenced directly from asm
    uint64_t rip;       // at the end of ist_usr stack an iretq
    uint64_t cs;        // structure, initialized by thread_new()
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} __attribute__((packed)) OSZ_tcb;

// compile time upper bound check
c_assert(sizeof(OSZ_tcb) == __PAGESIZE);

#endif
