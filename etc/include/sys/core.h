/*
 * sys/core.h
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
 * @brief OS/Z Core services (syscall numbers). Include with osZ.h
 */

#ifndef _SYS_CORE_H
#define _SYS_CORE_H 1

/*** low level codes in rdi for syscall instruction ***/
// rax: 00000000000xxxx Memory and threading
#define SYS_IRQ 0           // CORE sends it to device drivers, disables IRQ
#define SYS_ack 1           // device drivers to CORE, re-enable IRQ
#define SYS_nack 2          // negative acknowledge
#define SYS_dl 3
#define SYS_sched_yield 4
#define SYS_seterr 5
#define SYS_setirq 6
#define SYS_exit 7
#define SYS_swapbuf 8
#define SYS_regservice 9
#define SYS_stimebcd 10     // driver to CORE, cmos local date
#define SYS_stime 11
#define SYS_alarm 12
#define SYS_meminfo 13
#define SYS_mmap 14         // mman functions
#define SYS_munmap 15
#define SYS_mprotect 16
#define SYS_msync 17
#define SYS_mlock 18
#define SYS_munlock 19
#define SYS_mlockall 20
#define SYS_munlockall 21
#define SYS_mapfile 22      // process functions
#define SYS_fork 23
#define SYS_exec 24
#define SYS_sync 25

#define SYS_recv 0xFFFF     // receive message


// rax: FFFFFFFFFFFFxxxx File system services
// see sys/fs.h, stdlib.h, stdio.h, unistd.h
// rax: FFFFFFFFFFFExxxx User Interface services
// see sys/ui.h
// rax: FFFFFFFFFFFDxxxx Syslog services
// see sys/syslog.h, syslog.h
// rax: FFFFFFFFFFFCxxxx Networking services
// see sys/net.h, inet.h
// rax: FFFFFFFFFFFBxxxx Audio services
// see sys/sound.h
// rax: FFFFFFFFFFFAxxxx Init, user services
// see sys/init.h

/*** libc implementation prototypes */
#ifndef _AS
#include <sys/stat.h>

/*** message queue ***/
/* async, send a message (non-blocking, except dest queue is full) */
uint64_t mq_send(pid_t dst, uint64_t func, ...);
/* sync, send a message and receive result (blocking) */
msg_t *mq_call(pid_t dst, uint64_t func, ...);
/* async, is there a message? return serial or null (non-blocking) */
uint64_t mq_ismsg();
/* sync, wait until there's a message (blocking) */
msg_t *mq_recv();
/* sync, dispatch events (blocking, noreturn unless error) */
uint64_t mq_dispatch();

//thread-safe libc errno at an absolute address (in TCB). Read-only, use seterr()
extern uint16_t errno;

// bit manipulations
void lockacquire(int bit, uint64_t *ptr); // return only when the bit is set and was clear, yield otherwise
void lockrelease(int bit, uint64_t *ptr); // clear a bit

// Memory and threading
/* TODO: move these to unistd.h, leave only OS/Z specific calls here */
void *dl(uchar *sym, uchar *elf);       // dynamically link a symbol
void yield();                           // give up CPU time
void seterr(int errno);                 // set libc errno
void exit(int err);                     // exit task
uint64_t regservice(pid_t thread);      // register service (init only)
void stime(uint64_t utctimestamp);      // set system time
void setirq(int8_t irq);                // set irq message for this thread
void sleep(uint64_t sec);               // sleep the thread for sec
void usleep(uint64_t usec);             // sleep the thread for micro sec
msg_t *meminfo();                       // get memory info. msg_t.arg0=free pages, msg_t.arg1=total pages
// see <sys/mman.h> too
size_t mapfile(void *bss, char *fn);    // map a file on initrd
pid_t fork();                           // fork thread
pid_t exec(uchar *cmd);                 // start a new process in the background
void sync();                            // flush cache buffers

// rax: FFFFFFFFFFFAxxxx Init, user services
extern void start();
extern void stop();
extern void restart();
extern void status();

#endif

#endif /* sys/core.h */
