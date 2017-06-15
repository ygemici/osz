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

#include <errno.h>
#include <limits.h>
#ifndef _AS
#include <stdint.h>
#include <sys/types.h>
#include <sys/sysinfo.h>
#include "../../loader/bootboot.h"
#endif
#include <syscall.h>

#define FBUF_ADDRESS  0xffffffffe0000000
#define TMPQ_ADDRESS  0xffffffffffc00000
#define CORE_ADDRESS  0xffffffffffe02000

#define TCB_ADDRESS    0
#define FILE_ADDRESS   (BSS_ADDRESS-__SLOTSIZE)    // 4G-2M list of open files
#define BUF_ADDRESS    (0x00007fff00000000)        //128T-4G data, slot alloced buffers
#define SBSS_ADDRESS   (0xFFFF800000000000)        //shared memory, see bztalloc.c

#include "msg.h"
#include "env.h"

#ifndef _AS
#include "pmm.h"

// import virtual addresses from linker
extern BOOTBOOT bootboot;                     // boot structure
extern unsigned char environment[__PAGESIZE]; // configuration

extern uint8_t tmpmap;                // temporarily mapped page
extern uint8_t tmp2map;               // temporarily mapped page #2
extern uint8_t tmpalarm;              // temporarily mapped tcb for next alarm
extern uint8_t tmpctrl;               // control page for mapping tmpmap
extern uint8_t tmpmqctrl;             // temporarily mapped mq control page
extern uint8_t __bss_start;           // start of bss segment

// kernel variables
extern uint64_t *irq_routing_table;   // IRQ Routing Table
extern phy_t idle_mapping;            // memory mapping for "idle" task
extern OSZ_pmm pmm;                   // Physical Memory Manager data
extern int scry;                      // scroll counter for console

/* see etc/include/syscall.h */
extern pid_t *services;

/* size of the file returned by fs_locate() */
extern uint64_t fs_size;

// relocation records for runtime linker
typedef struct {
    uint64_t offs;
    char *sym;
} OSZ_rela;

// kernel function routines

// ----- Console -----
/** wait for a key by polling keyboard */
extern uint64_t kwaitkey();

/** Initialize console printf for debugging and panicing */
extern void kprintf_init();

/** Set default colors and move cursor home */
extern void kprintf_reset();

/** Scroll the screen */
extern void kprintf_scrollscr();

/** Print a formatted string to console */
extern void kprintf(char* fmt, ...);

/** Display a reason and die */
extern void kpanic(char *reason, ...);

// ----- Platform -----
/** Parse configuration to get environment */
extern void env_init();

/** Initialize Physical Memory Manager */
extern void pmm_init();

/** Allocate a physical memory page */
extern void *pmm_alloc();

/** Allocate a physical memory slot (2M) */
extern void *pmm_allocslot();

/** Map a specific memory area into thread's bss */
extern void vmm_mapbss(OSZ_tcb *tcb,virt_t bss, phy_t phys, size_t size, uint64_t access);


/** Initialize Interrupt Service Routines */
extern void isr_init();
extern void isr_fini();
extern void isr_enableirq(uint8_t irq);
extern void isr_disableirq(uint8_t irq);
extern int  isr_installirq(uint8_t irq, phy_t memroot);

// ----- System -----
/** Initialize system (idle thread and device drivers) */
extern void sys_init();

/** Switch to first task and start executing user space code */
extern void sys_enable();

/** Turn the computer off */
extern void sys_disable();

/** Reboot the computer */
extern void sys_reset();

/** called when idle is first scheduled */
extern void sys_ready();

// ----- File System -----
/** Initialize file system thread */
extern void fs_init();

/** Locate a file on initrd and return it's physical address
    NOTE: returned file's size is in fs_size, relies on identity mapping */
extern void *fs_locate(char *fn);

// ----- Syslog interface -----
/** Initialize syslog thread */
extern void syslog_init();
extern void syslog_early(char* fmt, ...);

// ----- User interface -----
/** Initialize user interface thread */
extern void ui_init();

// ----- Memory Management -----
/** Initialize kernel memory mapping */
extern void *kmap_init();

/** Map a physical page at a virtual address */
extern void kmap(virt_t virt, phy_t phys, uint8_t access);

/** temporarirly map a message queue at TMPQ_ADDRESS */
extern void kmap_mq(phy_t tcbmemroot);

/** return a pointer to PTE for virtual address */
extern uint64_t *kmap_getpte(virt_t virt);

/** Copy n bytes from src to desc */
extern void kmemcpy(void *dest, void *src, int size);

/** Copy n bytes from src to desc, other direction */
extern void kmemmove(void *dest, void *src, int size);

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

// ----- Threads -----
/** Allocate and initialize thread structures */
extern pid_t thread_new(char *cmdline);

/** Sanity check thread data */
extern bool_t thread_check(OSZ_tcb *tcb, phy_t *paging);

/** Check access for a group */
extern bool_t thread_allowed(char *grp, uint8_t access);

// ----- Scheduler -----
/** Add thread to scheduling */
extern void sched_add(OSZ_tcb *tcb);

/** Remove thread from scheduling */
extern void sched_remove(OSZ_tcb *tcb);

/** Block a thread */
extern void sched_block(OSZ_tcb *tcb);

/** Unblock a thread */
extern void sched_awake(OSZ_tcb *tcb);

/** suspend a thread until given time */
void sched_alarm(OSZ_tcb *tcb, uint64_t sec, uint64_t nsec);

/** Hybernate a thread */
extern void sched_sleep(OSZ_tcb *tcb);

/** Return next thread's memroot phy address */
extern phy_t sched_pick();

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
extern uint64_t service_register(pid_t thread);

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
extern void dbg_init();
extern void dbg_enable(virt_t rip, virt_t rsp, char *reason);
#endif

#endif

#endif /* _OSZ_CORE_H */
