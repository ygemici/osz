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
 * @brief Task Control Block, architecture specific part
 */

#include <limits.h>

/* WARNING: must match tcb struct, for asm */
#define tcb_priority 4
#define tcb_state 5
#define tcb_errno 6
#define tcb_gpr   8
#define tcb_fx  128
#define tcb_pid 640
#define tcb_memroot 648
#define tcb_rootdir 656
#define tcb_billcnt 664
#define tcb_excerr 680
#define tcb_serial 816

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
 * each field in the struct. The current task's tcb is at 0. */

// must be __PAGESIZE long
typedef struct {
    uint32_t magic;     // Task Control Block magic, must be 'TASK'
    uint8_t priority;   // task priority                     tcb+   4
    uint8_t state;      // task state                        tcb+   5
    uint8_t errno;      // thread safe libc errno            tcb+   6
    uint8_t forklevel;  // fork level                        tcb+   7
    uint8_t gpr[120];   // general purpose registers area at tcb+   8
    uint8_t fx[512];    // media registers save area at      tcb+ 128
    pid_t pid;          // task's pid at                     tcb+ 640
    phy_t memroot;      // memory mapping root at            tcb+ 648
    fid_t rootdir;      // inode of root directory           tcb+ 656
    uint64_t billcnt;   // ticks task spent                  tcb+ 664
    uint64_t cpu;       // APIC ID of executor cpu           tcb+ 672
    uint64_t excerr;    // exception error code              tcb+ 680
    pid_t sendto;       // destination task                  tcb+ 688
    pid_t blksend;      // head of tasks waiting to send     tcb+ 696
    pid_t next;         // next task in priority level       tcb+ 704
    pid_t prev;         // previous task in priority level   tcb+ 712
    pid_t alarm;        // next task in alarm queue          tcb+ 720
    uint64_t alarmsec;  // when to wake up (UTC timestamp)   tcb+ 728
    uint64_t alarmns;   // when to wake up (nanosec)         tcb+ 736
    uint64_t linkmem;   // number of linked memory pages     tcb+ 744
    uint64_t allocmem;  // number of allocated memory pages  tcb+ 752
    uint64_t self;      // self pointer (phy_t of TLB PT)    tcb+ 760
    uint64_t blktime;   // time when task was blocked        tcb+ 768
    uint64_t blkcnt;    // ticks task spent blocked          tcb+ 776
    uint64_t swapminor; // swap inode when hybernated        tcb+ 784
    uint64_t cmdline;   // pointer to argc in stack          tcb+ 792
    uint64_t name;      // pointer to argv[0] in stack       tcb+ 800
    uint32_t filesmax;  // number of maximum open files      tcb+ 808
    uint32_t memmax;    // number of maximum allocated pages tcb+ 812
    uint64_t serial;    // message serial                    tcb+ 816
    sighandler_t sighandler[32]; // for receiving signals    tcb+ 824
    uuid_t acl[TCB_MAXACE];// access control list            tcb+ 1080

    // compile time check for minimum stack size
    uint8_t padding[__PAGESIZE-TCB_MAXACE*16-TCB_ISRSTACK- 1080 ];
    uint8_t irqhandlerstack[TCB_ISRSTACK-40];

    // the last 40 bytes of stack is referenced directly from asm
    uint64_t rip;       // at the end of ist_usr stack an iretq
    uint64_t cs;        // structure, initialized by task_new()
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} __attribute__((packed)) tcb_t;

// compile time upper bound check
c_assert(sizeof(tcb_t) == __PAGESIZE);

#endif
