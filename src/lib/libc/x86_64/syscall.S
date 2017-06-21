/*
 * lib/libc/x86_64/syscall.S
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
 * @brief Low level library implementation to interact with the kernel, see etc/include/syscall.h
 */

#define _AS 1
#include <limits.h>
#include <syscall.h>
#include <errno.h>
#include "../../../core/x86_64/tcb.h"

.global mq_call
.global mq_send
.global mq_recv
.global mq_ismsg
.global lockacquire
.global lockrelease
.global dl
.global yield
.global seterr
.global setirq
.global mmap
.global munmap
.global mapfile
.global sleep
.global usleep
.global meminfo
#if DEBUG
.global dbg_printf
#endif

/* don't use directly, use mq_dispatch() instead */
.global mq_dispatchcall

.section .text

#if DEBUG
dbg_printf:
    xorq    %rax, %rax
    movq    %rcx, %rbx
    syscall
    ret
#endif

/* dest: pid_t<<16 + msg type */

/**
 * void mq_send(thread,event,arg0,arg1,arg2,arg3,arg4,arg5)
 */
mq_send:
mymq_send:
    # non-blocking send message
    incq    MQ_ADDRESS + 24
    movq    %rdi, %rax
    shlq    $16, %rax
    movw    %si, %ax
    movq    %rdx, %rdi
    movq    %rcx, %rsi
    movq    %r8, %rdx
    movq    %r9, %rbx
    movq    8(%rsp), %r8
    movq    16(%rsp), %r9
    /* this will call msg_sends() in core */
    syscall
    movq    MQ_ADDRESS + 24, %rax
    ret

/**
 * msg_t *mq_call(thread,event,arg0,arg1,arg2,arg3,arg4,arg5)
 */
mq_call:
mymq_call:
    # blocking, send a request and receive result
    /* send request */
1:  callq   mymq_send
    cmpb    $EAGAIN, tcb_errno
    jne     1f
    callq   myyield
    jmp     1b
1:  /* receive result */
    callq   mymq_recv
    ret

/**
 * msg_t *mq_recv()
 */
mq_recv:
mymq_recv:
    # blocking if there's no message waiting
1:  movq    $MQ_ADDRESS, %rbx
    movq    8(%rbx), %rax
    cmpq    (%rbx), %rax
    jne     2f
    /* block until we receive something */
    xorq    %rax, %rax
    movw    $SYS_recv, %ax
    syscall
    /* failsafe, repeat if there's no message */
    jmp      1b
2:  /* must match sizeof(msg_t) */
    movq    $MQ_ADDRESS, %rbx
    shlq    $6, %rax
    addq    %rbx, %rax
    /* mqhdr->mq_end++ */
    incq    8(%rbx)
    movq    8(%rbx), %rcx
    cmpq    16(%rbx), %rcx
    jne     3f
    movq    $1, 8(%rbx)
3:  ret

/**
 * returns message serial or 0 if no message waiting
 * uint64_t mq_ismsg()
 */
mq_ismsg:
    # non-blocking is there a message?
    movq    MQ_ADDRESS, %rax
    cmpq    MQ_ADDRESS+8, %rax
    je      1f
    movq    56(%rax), %rax
    orq     %rax, %rax
    jnz     1f
    incb    %al
1:  ret

/* Don't use directly
   uint64_t mq_displatchcall(rdi a0,rsi a1,rdx a2,rcx a3,r8 a4,r9 a5,stack func); */
mq_dispatchcall:
    /* rax=*symtable[MSG_FUNC(msg->evt)](msg->arg0, msg->arg1, msg->arg2, msg->arg3, msg->arg4, msg->arg5) */
    movq    8(%rsp), %rax
    callq   *%rax
    ret

/**
 * void lockacquire(int bit, uint64_t *mem)
 */
lockacquire:
1:  lock
    btsq    %rdi, (%rsi)
    jnc     1f
    callq   myyield
    jmp     1b
1:  ret

/**
 * void lockrelease(int bit, uint64_t *mem)
 */
lockrelease:
    lock
    btrq    %rdi, (%rsi)
    ret

/**
 * void *dl(so, sym)
 */
dl:
    # dynamically link a shared object
    # rdi: pointer to .so file path
    # rsi: pointer to symbol name
    xorq    %rax, %rax
    movb    $SYS_dl, %al
    syscall
    call    mymq_recv
    mov     8(%rax), %rax
    ret

/**
 * void yield()
 */
yield:
myyield:
    xorq    %rax, %rax
    movb    $SYS_sched_yield, %al
    syscall
    ret

/**
 * void seterr(uint64_t err)
 */
seterr:
myseterr:
    xorq    %rax, %rax
    movb    $SYS_seterr, %al
    syscall
    ret

/**
 * uint64_t regservice(pid_t thread) - ONLY for init
 */
regservice:
    xorq    %rax, %rax
    movb    $SYS_regservice, %al
    syscall
    ret

/**
 * void *mmap (void *addr, size_t len, int prot, int flags, int fd, off_t offset);
 */
mmap:
    movq    %rcx, %rbx
    xorq    %rax, %rax
    movb    $SYS_mmap, %al
    syscall
    ret

/**
 * int munmap (void *addr, size_t len);
 */
munmap:
    xorq    %rax, %rax
    movb    $SYS_munmap, %al
    syscall
    ret

/**
 * int mprotect (void *addr, size_t len, int prot);
 */
mprotect:
    xorq    %rax, %rax
    movb    $SYS_munmap, %al
    syscall
    ret

/**
 * size_t mapfile(void *bss, char *fn)
 */
mapfile:
    xorq    %rax, %rax
    movb    $SYS_mapfile, %al
    syscall
    ret

/**
 * void setirq(uint8_t irq) - ONLY for drivers
 */
setirq:
    andq    $0xFF, %rdi
    xorq    %rax, %rax
    movb    $SYS_setirq, %al
    syscall
    ret

/**
 * void sleep(uint64_t sec)
 */
sleep:
    xorq    %rax, %rax
    movb    $SYS_alarm, %al
    syscall
    ret

/**
 * void usleep(uint64_t usec)
 */
usleep:
    xorq    %rdx, %rdx
    movq    %rdi, %rax
    movq    $1000000,%rcx
    divq    %rcx
    movq    %rax, %rdi
    movq    %rdx, %rsi
    xorq    %rax, %rax
    movb    $SYS_alarm, %al
    syscall
    ret

/**
 * msg_t *meminfo()
 */
meminfo:
    xorq    %rax, %rax
    movb    $SYS_meminfo, %al
    syscall
    callq   mymq_recv
    ret
