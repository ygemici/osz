/*
 * core/core.h
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
 * @brief Core functions (ring 0)
 */

#ifndef _OSZ_CORE_H
#define _OSZ_CORE_H

#define PHYMEM_MIN  32 //Mb, minimum RAM required

#include <errno.h>
#include <limits.h>
#ifndef _AS
#include <stdint.h>
#include <stdarg.h>
#include <sys/types.h>
#include "../../loader/bootboot.h"
#endif
#include <syscall.h>
#include <sys/debug.h>

#define INITRD_ADDRESS 0xffffffffc0000000   //initrd map address
#define VMM_ADDRESS    0xffffffffc1000000   //temporarily mapped vmm tables
#define FBUF_ADDRESS   0xfffffffffc000000   //framebuffer address
#define TMPQ_ADDRESS   0xffffffffffa00000   //temporarily mapped message queue
#define CORE_ADDRESS   0xffffffffffe02000   //core text segment

#define TCB_ADDRESS    0                    //Task Control Block for current task
#define BUF_ADDRESS    0x0000007f00000000   //512G-4G data, slot alloc'd buffers

#define USERSTACK_MAX 256 //kbytes
#define NRMQ_MAX      ((TEXT_ADDRESS-MQ_ADDRESS-(USERSTACK_MAX*1024))/__PAGESIZE)

/* ticks indeces for counters */
#define TICKS_TS  0     //+00 timestamp sec counter
#define TICKS_NTS 1     //+08 timestamp nanosec fraction
#define TICKS_LO  2     //+16 overall ticks (jiffies, 128 bit)
#define TICKS_HI  3     //+24

#include "msg.h"
#include "env.h"

/* locks. Currently only one is used for physical memory allocation */
#define LOCK_PMM    (1<<0)

#ifndef _AS
#include "pmm.h"

// import virtual addresses from linker
extern BOOTBOOT bootboot;                     // boot structure
extern unsigned char environment[__PAGESIZE]; // configuration

extern uint8_t tmpmap;                // temporarily mapped page
extern uint8_t tmp2map;               // temporarily mapped page #2
extern uint8_t tmp3map;               // temporarily mapped page #3
extern uint8_t tmpfx;                 // temporarily mapped tcb for last fx
extern uint8_t tmpalarm;              // temporarily mapped tcb for next alarm
extern uint8_t tmpctrl;               // control page for mapping tmpmap
extern uint8_t tmpmqctrl;             // temporarily mapped mq control page
extern uint8_t __dynbss_start;        // start of bss segment
extern ccb_t ccb;                     // CPU Control Block, mapped per core

// kernel variables
extern uint64_t *irq_routing_table;   // IRQ Routing Table
extern pmm_t pmm;                     // Physical Memory Manager data
extern int scry;                      // scroll counter for console
extern uint8_t sys_fault;             // system fault code
extern char *syslog_buf;              // syslog buffer
extern char *drvs;                    // device drivers map
extern char *drvs_end;
extern phy_t idle_mapping;

/* see etc/include/syscall.h */
extern pid_t services[NUMSRV];

/* size of the file returned by fs_locate() */
extern uint64_t fs_size;

// relocation records for runtime linker
typedef struct {
    uint64_t offs;
    char *sym;
} rela_t;

// common data
extern uint32_t numcores;
extern uint64_t srand[4];
extern uint64_t ticks[4];
extern uint64_t systables[8];

// kernel function routines

// ----- Core sync -----
/** acquire a lock, spin loop until it's ok */
extern void klockacquire(int bit);
/** release a lock */
extern void klockrelease(int bit);

// ----- Console -----
/** Initialize console printf for debugging and panicing */
extern void kprintf_init();

/** Set default colors and move cursor home */
extern void kprintf_reset();
extern void kprintf_fg(uint32_t color);
extern void kprintf_bg(uint32_t color);

/** fade the screen and reset cursor */
extern void kprintf_fade();

/** Scroll the screen */
extern void kprintf_scrollscr();

/** Print a formatted string to console */
extern void kprintf(char* fmt, ...);

/** Display a reason and die */
extern void kpanic(char *reason, ...);

/** Display good bye screen */
extern void kprintf_poweroff();
extern void kprintf_reboot();

// ----- Platform -----
extern uint64_t platform_waitkey(); // wait for a key by polling keyboard
extern void platform_env();         // called by env_init()
extern void platform_detect();      // called by sys_init() before isr_init()
extern void platform_enumerate();   // called by sys_init()
extern void platform_timer();       // called by isr_init()
extern virt_t platform_mapdrvmem(); // called by drv_init()
extern void platform_poweroff();    // called by kprintf_poweroff()
extern void platform_reset();       // called by kprintf_reset()
extern void platform_halt();        // hang the system
extern void platform_srand();       // initialize random seed on the platform
#if DEBUG
extern void platform_dbginit();     // initialize debug console
extern void platform_dbgputc(uint8_t c);// display a character on debug console
#endif

/** Parse configuration to get environment */
extern void env_init();

/** Initialize Physical Memory Manager */
extern void pmm_init();

/** Allocate continous physical memory */
extern void *pmm_alloc(int pages);

/** Allocate a physical memory slot (2M) */
extern void *pmm_allocslot();

/** Free physical memory */
extern void pmm_free(phy_t base, size_t numpages);

/** Initialize kernel memory mapping */
extern uint64_t *vmm_init();

/** Map a physical page at a virtual address */
extern void vmm_map(virt_t virt, phy_t phys, uint16_t access);

/** temporarily map a message queue at TMPQ_ADDRESS */
extern void vmm_mq(phy_t tcbmemroot);

/** temporarily map a buffer (up to 2M) at TMPQ_ADDRESS */
extern void vmm_buf(phy_t tcbmemroot, virt_t ptr, size_t size);

/** return a pointer to PTE for virtual address */
extern phy_t *vmm_getpte(virt_t virt);

/** Allocate and initialize a new address space */
extern tcb_t *vmm_idletask();
extern tcb_t *vmm_newtask(char *cmdline, uint8_t prio);
/** Map vmm tables for an existing address space */
extern tcb_t *vmm_maptask(phy_t memroot);

/*** set up the main entry point */
extern void vmm_setexec(virt_t func);
/*** push an entry point for execution */
extern void vmm_pushentry(virt_t func);

/** Sanity check process data */
extern bool_t vmm_check();
extern void vmm_checklastpt();

/** map core buffer in task's address space */
extern virt_t vmm_mapbuf(void *buf, uint64_t npages, uint64_t access);

/** Map a specific memory area into task's bss */
extern void vmm_mapbss(tcb_t *tcb,virt_t bss, phy_t phys, size_t size, uint64_t access);

/** Unmap task's bss */
extern void vmm_unmapbss(tcb_t *tcb,virt_t bss, size_t size);

/** Switch to an address space (macro) */
//void vmm_switch(memroot);

/** Switch to an adress space and execute it (macro) */
//void vmm_enable(memroot, func, stack);

/** Initialize Interrupt Service Routines */
extern void isr_init();
extern void isr_fini();
extern void isr_enableirq(uint16_t irq);
extern void isr_disableirq(uint16_t irq);
extern int  sys_installirq(uint16_t irq, phy_t memroot);

// ----- System -----
/** Initialize main system "service" (idle task and device drivers) */
extern void sys_init();

/** Switch to first task and start executing user space code */
extern void sys_enable();

/** called when idle is first scheduled */
extern void sys_ready();

// ----- File System -----
/** Initialize file system task */
extern void fs_init();

/** Locate a file on initrd and return it's physical address
    NOTE: returned file's size is in fs_size, relies on identity mapping */
extern void *fs_locate(char *fn);

// ----- Syslog interface -----
/** Initialize syslog task */
extern void syslog_init();
extern void syslog_early(char* fmt, ...);

// ----- User interface -----
/** Initialize user interface task */
extern void ui_init();

// ----- Memory Management -----
/** Copy n bytes from src to desc */
extern void kmemcpy(void *dest, void *src, int size);

/** Set n bytes of memory to character */
extern void kmemset(void *dest, int c, int size);

/** Compare n bytes of memory */
extern int kmemcmp(void *dest, void *src, int size);

/** Copy a zero terminated string */
extern char *kstrcpy(char *dest, char *src);

/** Return string length */
extern int kstrlen(char *src);

/** Compare two zero terminated strings */
extern int kstrcmp(void *dest, void *src);

/** Allocate kernel bss, return virtual address */
extern void* kalloc(int pages);

/** Free kernel bss */
extern void kfree(void* ptr);

/** Add entropy to random generator **/
extern void kentropy();

// ----- Security -----
/** Check access for a group */
extern bool_t task_allowed(tcb_t *tcb, char *grp, uint8_t access);

/** Check if msg can be sent */
extern bool_t msg_allowed(tcb_t *sender, pid_t dest, evt_t event);


// ----- Scheduler -----
/** Add task to scheduling */
extern void sched_add(tcb_t *tcb);

/** Remove task from scheduling */
extern void sched_remove(tcb_t *tcb);

/** Block a task */
extern void sched_block(tcb_t *tcb);

/** Unblock a task */
extern void sched_awake(tcb_t *tcb);

/** suspend a task until given time */
extern void sched_alarm(tcb_t *tcb, uint64_t sec, uint64_t nsec);

/** Hybernate a task */
extern void sched_sleep(tcb_t *tcb);

/** Return next task's memroot phy address */
extern phy_t sched_pick();

#if DEBUG
extern void sched_dump();
#endif

// ----- Sybsystem Management -----
/** Load an ELF binary into address space, return physical address */
extern void *elf_load(char *fn);

/** Load a shared library into address space */
extern void elf_loadso(char *fn);

/** Parse dynamic header to find out which shared objects to load */
extern void elf_neededso();

/** Run-time link ELF binaries in address space */
extern bool_t elf_rtlink();

/** Initialize a subsystem, a system service */
extern void service_init(int subsystem, char *fn);

/** Register a user service */
extern uint64_t service_register(pid_t task);

/** Initialize a device driver service */
extern void drv_init(char *fn);

// ----- Message Queue -----
/** normal message senders */
/* send a message with pointer+size pair */
#define msg_send(event,ptr,size,magic) msg_sends(MSG_PTRDATA | event, (uint64_t)ptr, (uint64_t)size, magic, 0, 0, 0)
/* send a message with scalar values */
extern uint64_t msg_sends(evt_t event, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5);
/** low level message sender, called by senders above and IRQ ISRs */
extern uint64_t ksend(msghdr_t *mqhdr, evt_t event, uint64_t arg0, uint64_t arg1, uint64_t arg2,uint64_t arg3, uint64_t arg4, uint64_t arg5);

#if DEBUG
/* enable internal debugger */
extern void dbg_enable(virt_t rip, virt_t rsp, char *reason);
/* put a character on serial line */
extern void dbg_putchar(uint8_t c);
#endif

#endif

#endif /* _OSZ_CORE_H */
