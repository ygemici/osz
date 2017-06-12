/*
 * libc/x86_64/syscall.S
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

.global mq_call
.global mq_send
.global mq_recv
.global mq_ismsg
.global dl
.global sysinfo
.global yield
.global seterr
.global setirq
.global mapfile
.global sleep
.global usleep

/* don't use directly, use mq_dispatch() instead */
.global mq_dispatchcall

.section .text

/* dest: pid_t<<16 + msg type */

/**
 * void mq_send(thread,event,arg0,arg1,arg2,arg3,arg4,arg5)
 */
mq_send:
    # non-blocking send message
    /* this will call msg_sends() in core */
    movq     %rcx, %r10
    movl     $0x646E6573, %eax # 'send'
    syscall
    ret

/**
 * msg_t *mq_call(thread,event,arg0,arg1,arg2,arg3,arg4,arg5)
 */
mq_call:
mymq_call:
    # blocking, send a request and receive result
    # rdi: destination pid + message type
    /* must match OSZ_tcb, tcb->recvfrom */
    movq     %rdi, %rax
    shrq     $16, %rax
    /* lock message queue */
    movl     %eax, MQ_ADDRESS + 24
    shrq     $32, %rax
    movswl   %ax, %eax
    movl     %eax, MQ_ADDRESS + 28
    movq     %rcx, %r10
    movl     $0x6c6c6163, %eax # 'call'
    syscall
    /* unlock */
    movq     $0, MQ_ADDRESS + 24
    /* receive result. Stupid GNU ld needs another label to avoid relocation
       for in text segment calls... */
    call    mymq_recv
    movq    8(%rax), %rax
    ret

/**
 * msg_t *mq_recv(from)
 */
mq_recv:
mymq_recv:
    # blocking receive message
1:  movq     $MQ_ADDRESS, %rbx
    movq     8(%rbx), %rax
    cmpq     (%rbx), %rax
    jne      2f
    movq     %rcx, %r10
    movl     $0x76636572, %eax # 'recv'
    /* block until we receive something */
    syscall
    /* failsafe, repeat if there's no message */
    jmp      1b
2:  /* must match sizeof(msg_t) */
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
 * bool_t mq_ismsg()
 */
mq_ismsg:
    # non-blocking is there a message?
    movq     MQ_ADDRESS, %rax
    subq     MQ_ADDRESS+8, %rax
    ret

/* Don't use directly
   uint64_t mq_displatchcall(rdi a0,rsi a1,rdx a2,rcx a3,r8 a4,r9 a5,stack func); */
mq_dispatchcall:
    /* rax=*symtable[MSG_FUNC(msg->evt)](msg->arg0, msg->arg1, msg->arg2, msg->arg3, msg->arg4, msg->arg5) */
    movq    8(%rsp), %rax
    callq   *%rax
    ret

/**
 * void *dl(so, sym)
 */
dl:
    # dynamically link a shared object
    # rdi: pointer to .so file path
    # rsi: pointer to symbol name
    movq    %rsi, %rdx
    movq    %rdi, %rsi
    xorq    %rdi, %rdi
    movb    $SYS_dl, %dil
    movl    $0x646E6573, %eax # 'send'
    syscall
    jmp     2f

/**
 * sysinfo_t *sysinfo()
 */
sysinfo:
    xorq    %rdi, %rdi
    movb    $SYS_sysinfo, %dil
    movl    $0x646E6573, %eax # 'send'
    syscall
2:  call    mymq_recv
    mov     8(%rax), %rax
    ret

/**
 * void yield()
 */
yield:
    xorq    %rdi, %rdi
    movb    $SYS_sched_yield, %dil
    movl    $0x646E6573, %eax # 'send'
    syscall
    ret

/**
 * void seterr(uint64_t err)
 */
seterr:
myseterr:
    movq    %rdi, %rsi
    xorq    %rdi, %rdi
    movb    $SYS_seterr, %dil
    movl    $0x646E6573, %eax # 'send'
    syscall
    ret

/**
 * uint64_t regservice(pid_t thread) - ONLY for init
 */
regservice:
    movq    %rdi, %rsi
    xorq    %rdi, %rdi
    movb    $SYS_regservice, %dil
    movl    $0x646E6573, %eax # 'send'
    syscall
    ret

/**
 * size_t mapfile(void *bss, char *fn)
 */
mapfile:
    movq    %rsi, %rdx
    movq    %rdi, %rsi
    xorq    %rdi, %rdi
    movb    $SYS_mapfile, %dil
    movl    $0x646E6573, %eax # 'send'
    syscall
    ret

/**
 * void setirq(uint8_t irq) - ONLY for drivers
 */
setirq:
    xorq    %rsi, %rsi
    movb    %dil, %sil
    xorq    %rdi, %rdi
    movb    $SYS_setirq, %dil
    movl    $0x646E6573, %eax # 'send'
    syscall
    ret

/**
 * void sleep(uint64_t sec)
 */
sleep:
    xorq    %rdx, %rdx
    movq    %rdi, %rsi
    xorq    %rdi, %rdi
    movb    $SYS_alarm, %dil
    movl    $0x646E6573, %eax # 'send'
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
    movq    %rax, %rsi
    xorq    %rdi, %rdi
    movb    $SYS_alarm, %dil
    movl    $0x646E6573, %eax # 'send'
    syscall
    ret
