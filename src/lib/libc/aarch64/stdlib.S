/*
 * lib/libc/aarch64/stdlib.S
 *
 * Copyright 2016 CC-by-nc-sa-4.0 bztsrc@github
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
 * @brief Low level library implementation to interact with the kernel, see stdlib.h
 */

#define _AS 1
#include <limits.h>
#include <syscall.h>
#include <errno.h>
#include "../../../core/aarch64/tcb.h"

.global mq_call
.global mq_send
.global mq_recv
.global mq_ismsg
.global p2pcpy
.global lockacquire
.global lockrelease
.global dl
.global yield
.global setirq
.global mmap
.global munmap
.global p2pcpy
.global sleep
.global usleep
.global meminfo
.global getentropy
.global abort
.global exit
.global srand
.global rand
.global mblen
.global div
.global ldiv
.global lldiv
/* don't use directly, use mq_dispatch() instead */
.global mq_dispatchcall
#if DEBUG
.global dbg_printf
#endif

.section .text
/*****************************************************************************
 *                             OS/Z system calls                             *
 *****************************************************************************/
 
/**
 * non-blocking, send message
 *
 * void mq_send(task,event,arg0,arg1,arg2,arg3,arg4,arg5)
 */
mq_send:
mymq_send:
    ret

/**
 * blocking, send a request and receive result
 *
 * msg_t *mq_call(task,event,arg0,arg1,arg2,arg3,arg4,arg5)
 */
mq_call:
mymq_call:
1:  ret

/**
 * blocking if there's no message waiting
 *
 * msg_t *mq_recv()
 */
mq_recv:
mymq_recv:
    ret

/**
 * returns message serial or 0 if no message waiting
 *
 * uint64_t mq_ismsg()
 */
mq_ismsg:
1:  ret

/*****************************************************************************
 *                      OS/Z standard library functions                      *
 *****************************************************************************/

#if DEBUG
dbg_printf:
    ret
#endif

/* Don't use directly */
mq_dispatchcall:
    /* rax=*symtable[MSG_FUNC(msg->evt)](msg->arg0, msg->arg1, msg->arg2, msg->arg3, msg->arg4, msg->arg5) */
    ret

/* Don't use directly. in: rbx: errno */
myseterrno:
    ret

/**
 * void p2pcpy (pid_t dst, void* dest, void *src, size_t len);
 */
p2pcpy:
    ret

/**
 * Set a bit. Return if it was zero, yield otherwise.
 *
 * void lockacquire(int bit, uint64_t *mem)
 */
lockacquire:
1:  ret

/**
 * Clear a bit
 *
 * void lockrelease(int bit, uint64_t *mem)
 */
lockrelease:
    ret

/**
 * Dynamically link a shared object
 *
 * void *dl(so, sym)
 */
dl:
    # rdi: pointer to .so file path
    # rsi: pointer to symbol name
    ret

/**
 * Give up remaining CPU time in this quantum
 *
 * void yield()
 */
yield:
myyield:
    ret

/**
 * void *mmap (void *addr, size_t len, int prot, int flags, int fd, off_t offset);
 */
mmap:
    ret

/**
 * int munmap (void *addr, size_t len);
 */
munmap:
    ret

msync:
    ret

mlock:
    ret

munlock:
    ret

/**
 * void setirq(uint8_t irq) - ONLY for drivers
 */
setirq:
    ret

/**
 * void sleep(uint64_t sec)
 */
sleep:
    ret

/**
 * void usleep(uint64_t usec)
 */
usleep:
    ret

/**
 * msg_t *meminfo()
 */
meminfo:
    ret

/**
 * int getentropy (void *__buffer, size_t __length);
 */
getentropy:
    /* TODO: getentropy */
    ret

/**
 * Exit process.
 *
 * void exit(status)
 */
exit:
    /* fall into abort code. If we return something really bad happened */
/**
 * Abort execution and generate a core-dump.
 *
 * void abort() 
 */
abort:
    ret

/**
 * initialize random with seed
 *
 * void srand(uint64_t seed)
 */
srand:
    ret

/**
 * Fills up almost every register with random
 *
 * uint64_t rand()
 */
rand:
    ret

/**
 * int mblen (char *s, size_t n);
 */
mblen:
    ret

/**
 * div_t div (int numer, int denom);
 */
div:
    ret

/**
 * ldiv_t ldiv (long int numer, long int denom);
 * lldiv_t lldiv (long long int numer, long long int denom);
 */
ldiv:
lldiv:
    ret
