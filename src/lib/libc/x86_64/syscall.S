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
 * @brief Low base library to interact with the kernel at object level
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
.global swapbuf
.global mapfile

/* don't use directly, use mq_dispatch() instead */
.global mq_dispatchcall

.section .text

/* dest: pid_t<<16 + msg type */

/* void mq_send(thread,event,arg0,arg1,arg2,arg3,arg4,arg5) */
mq_send:
    # non-blocking send message
    /* this will call msg_sends() in core */
    movl     $0x646E6573, %eax # 'send'
    syscall
    ret

/* msg_t *mq_call(thread,event,arg0,arg1,arg2,arg3,arg4,arg5) */
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
    movl     $0x6c6c6163, %eax # 'call'
    syscall
    /* unlock */
    movq     $0, MQ_ADDRESS + 24
    /* receive result. Stupid GNU ld needs another label to avoid relocation
       for in text segment calls... */
    call    mymq_recv
    movq    8(%rax), %rax
    ret

/* msg_t *mq_recv(from) */
mq_recv:
mymq_recv:
    # blocking receive message
1:  movq     $MQ_ADDRESS, %rbx
    movq     8(%rbx), %rax
    cmpq     (%rbx), %rax
    jne      2f
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

/* bool_t mq_ismsg() */
mq_ismsg:
    # non-blocking is there a message?
    movq     MQ_ADDRESS, %rax
    subq     MQ_ADDRESS+8, %rax
    ret

/* uint64_t mq_displatchcall(Elf64_Sym *symtable, msg_t msg); */
mq_dispatchcall:
    /* TODO: rax=*symtable[MSG_FUNC(msg->evt)](msg->arg0, msg->arg1, msg->arg2, msg->arg3, msg->arg4, msg->arg5) */
    xorq    %rax, %rax
    /* tcb->errno = rax */
    movq    $6, %rdx
    movw    %ax, (%rdx)
    ret

/* void *dl(so, sym) */
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

/* sysinfo_t *sysinfo() */
sysinfo:
    xorq    %rdi, %rdi
    movb    $SYS_sysinfo, %dil
    movl    $0x646E6573, %eax # 'send'
    syscall
2:  call    mymq_recv
    mov     16(%rax), %rax
    ret

/* void yield() */
yield:
    xorq    %rdi, %rdi
    movb    $SYS_sched_yield, %dil
    movl    $0x646E6573, %eax # 'send'
    syscall
    ret

/* void seterr(uint64_t err) */
seterr:
    movq    %rdi, %rsi
    xorq    %rdi, %rdi
    movb    $SYS_seterr, %dil
    movl    $0x646E6573, %eax # 'send'
    syscall
    ret

/* void swapbuf() */
swapbuf:
    movq    %rdi, %rsi
    xorq    %rdi, %rdi
    movb    $SYS_swapbuf, %dil
    movl    $0x646E6573, %eax # 'send'
    syscall
    ret

/* uint64_t regservice(pid_t thread) */
regservice:
    movq    %rdi, %rsi
    xorq    %rdi, %rdi
    movb    $SYS_regservice, %dil
    movl    $0x646E6573, %eax # 'send'
    syscall
    ret

/* size_t mapfile(void *bss, char *fn) */
mapfile:
    movq    %rsi, %rdx
    movq    %rdi, %rsi
    xorq    %rdi, %rdi
    movb    $SYS_mapfile, %dil
    movl    $0x646E6573, %eax # 'send'
    syscall
    ret
