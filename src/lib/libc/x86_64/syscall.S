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

.global clcall
.global clsend
.global clrecv
.global clismsg
.global cldl

.section .text.user

/* dest: pid_t<<16 + msg type */

/* void clsend(thread,event,arg0,arg1,arg2) */
clsend:
    # non-blocking send message
    /* this will call msg_sendreg() or msg_sendptr() in core */
    movl     $0x646E6573, %eax # 'send'
    syscall
    ret

/* msg_t *clcall(thread,event,arg0,arg1,arg2) */
clcall:
    # blocking, send a request and receive result
    # rdi: destination pid + message type
    /* must match OSZ_tcb, tcb->recvfrom */
    movq     %rdi, MQ_ADDRESS + 24
    movl     $0x646E6573, %eax # 'send'
    syscall
    movq     $0, MQ_ADDRESS + 24
    /* fall into to clrecv code */

/* msg_t *clrecv(from) */
clrecv:
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
    shlq    $5, %rax
    addq    %rbx, %rax
    /* mqhdr->mq_end++ */
    incq    8(%rbx)
    movq    8(%rbx), %rcx
    cmpq    16(%rbx), %rcx
    jne     3f
    movq    $1, 8(%rbx)
3:  movq    (%rax), %rdi
    movq    8(%rax), %rsi
    movq    16(%rax), %rdx
    movq    24(%rax), %rcx
    ret

/* uint64_t clismsg() */
clismsg:
    # non-blocking is there a message?
    movq     MQ_ADDRESS, %rax
    subq     MQ_ADDRESS+8, %rax
    ret

/* void *cldl(&symtable,objectfile) */
cldl:
    # dynamically link a shared object
    # rdi: pointer to symtable to fill
    # rsi: pointer to .so file
    movl     $0x6B6E696C, %eax # 'link'
    syscall
    ret
