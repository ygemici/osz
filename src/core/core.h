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

#include <osZ.h>
#include <limits.h>
#include "../../loader/bootboot.h"
#include "pmm.h"
#include "msg.h"

// device drivers
typedef struct {
    char *fn;
    void *elf;
} drv_t;

// import virtual addresses from linker
extern BOOTBOOT bootboot;             // boot structure
extern uchar environment[__PAGESIZE]; // configuration
extern uint8_t fb;                    // framebuffer
extern uint8_t tmpmap;                // tempprarily mapped page
extern uint8_t tmpctrl;               // control page for mapping tmpmap
extern uint64_t tmppde;               // core's pde ptr in pdpe
extern msghdr_t tmpmq;                // destination message queue

#define USER_PROCESS SRV_init
#define OFFS_system (&subsystems[SRV_system])

// kernel variables

/* see etc/include/syscall.h */
extern pid_t subsystems[];

/* size of the file returned by fs_locate() */
extern uint64_t fs_size;

// kernel function routines

// ----- Console -----
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
/** Check CPU */
extern void cpu_init();

/** Parse configuration to get environment */
extern void env_init();

/** Initialize Physical Memory Manager */
extern void pmm_init();

/** Allocate a physical memory page */
extern void *pmm_alloc();

/** Initialize Interrupt Service Routines */
extern void isr_init();
extern void isr_enableirq(uint8_t irq);
extern void isr_disableirq(uint8_t irq);
extern void isr_gainentropy();

/** Enable interrupts and start multitasking */
extern void isr_enable();

// ----- System -----
/** Initialize system thread */
extern void sys_init();

/** Turn the computer off */
extern void sys_poweroff();


// ----- File System -----
/** Initialize file system thread */
extern void fs_init();

/** Locate a file on initrd and return it's physical address
    NOTE: returned file's size is in fs_size */
extern void *fs_locate(char *fn);

// ----- User interface -----
/** Initialize user interface thread */
extern void ui_init();

// ----- Memory Management -----
/** Initialize kernel memory mapping */
extern void *kmap_init();

/** Copy n bytes from src to desc */
extern void kmemcpy(char *dest, char *src, int size);

/** Set n bytes of memory to character */
extern void kmemset(char *dest, int c, int size);

/** Compare n bytes of memory */
extern int kmemcmp(void *dest, void *src, int size);

/** Copy a zero terminated string */
extern void kstrcpy(char *dest, char *src);

/** Return string length */
extern int kstrlen(char *src);

/** Compare two zero terminated strings */
extern int kstrcmp(void *dest, void *src);

/** Allocate kernel bss, return virtual address */
extern void* kalloc(int pages);

/** Free kernel bss */
extern void kfree(void* ptr);

/** Map a physical page at a virtual address */
extern void kmap(uint64_t virt, uint64_t phys, uint8_t access);

// ----- Threads -----
/** Allocate and initialize thread structures */
extern pid_t thread_new(char *cmdline);

// ----- Scheduler -----
/** Add thread to scheduling */
extern void sched_add(pid_t thread);

/** Remove thread from scheduling */
extern void sched_remove(pid_t thread);

/** Block a thread */
extern void sched_block(pid_t thread);

/** Unblock a thread */
extern void sched_activate(pid_t thread);

/** Return next thread's memroot */
extern uint64_t sched_pick();

// ----- Sybsystem Management -----
/** Load an ELF binary into address space, return physical address */
extern void *service_loadelf(char *fn);

/** Load a shared library into address space */
extern void service_loadso(char *fn);

/** Run-time link ELF binaries in address space */
extern void service_rtlink();

/** Map a specific memory area into user bss */
extern void service_mapbss(uint64_t phys, uint64_t size);

/** Initialize a subsystem, a system service */
extern void service_init(int subsystem, char *fn);

// ----- Message Queue -----
/** normal message senders */
extern bool_t msg_send(pid_t thread, uint64_t event, void *ptr, size_t size, uint64_t magic);
extern bool_t msg_sends(pid_t thread, uint64_t event, uint64_t arg0, uint64_t arg1, uint64_t arg2);
#define msg_sendsrv(service, event, ptr, size, magic) \
    msg_send(((service)&0xfff)|(uint64_t)0xfffffffffffff000, event, ptr, size, magic)
#define msg_sendssrv(service, event, arg0, arg1, arg2) \
    msg_sends(((service)&0xfff)|(uint64_t)0xfffffffffffff000, event, arg0, arg1, arg2)
/** low level message sender, called by senders above and IRQ ISRs */
extern bool_t ksend(msghdr_t *mqhdr, uint64_t event, uint64_t arg0, uint64_t arg1, uint64_t arg2);
