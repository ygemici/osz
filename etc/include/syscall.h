/*
 * syscall.h
 *
 * Copyright 2017 CC-by-nc-sa bztsrc@github
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
 * @brief OS/Z syscall services
 */

#ifndef _SYSCALL_H
#define _SYSCALL_H 1

/**
 * Normally this header should hold the system call defines, but
 * since OS/Z is a microkernel, it has several system services and
 * the system functions are spread accross them.
 */

/*** system services (negative and zero pid_t) ***/
#define SRV_CORE		 0
#define SRV_FS			-1
#define SRV_UI			-2
#define SRV_syslog		-3
#define SRV_inet		-4
#define SRV_sound		-5
#define SRV_init		-6
#define SRV_video		-7

#define NUMSRV          8

/*** get function indeces ***/
// 000000000000xxxx Memory, Multitask and other core services, stdlib.h
#include <sys/core.h>
// FFFFFFFFFFFFxxxx File system services, stdio.h
#include <sys/fs.h>
// FFFFFFFFFFFExxxx User Interface services, ui.h
#include <sys/ui.h>
// FFFFFFFFFFFDxxxx Syslog services, syslog.h
#include <sys/syslog.h>
// FFFFFFFFFFFCxxxx Networking services, inet.h
#include <sys/inet.h>
// FFFFFFFFFFFBxxxx Audio services, sound.h
#include <sys/sound.h>
// FFFFFFFFFFFAxxxx Init, control user services, init.h
#include <sys/init.h>

/*** libc implementation prototypes */
#ifndef _AS

/*** message queue, the real system calls ***/
/* async, send a message (non-blocking, except dest queue is full) */
uint64_t mq_send(pid_t dst, uint64_t func, ...);
/* sync, send a message and receive result (blocking) */
msg_t *mq_call(pid_t dst, uint64_t func, ...);
/* async, is there a message? return serial or 0 (non-blocking) */
uint64_t mq_ismsg();
/* sync, wait until there's a message (blocking) */
msg_t *mq_recv();
/* sync, dispatch events (blocking, noreturn unless error) */
uint64_t mq_dispatch();

#endif

#endif /* syscall.h */
