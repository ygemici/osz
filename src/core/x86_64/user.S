/*
 * core/x86_64/user.S
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
 * @brief User mode IRQ dispatcher
 */

#define _AS 1
#include <limits.h>
#include <syscall.h>
#include "isr.h"

.section .text.user0
.global _init
.global _main

/* load this page instead of /sbin/system in sys_init() */

/* fake crt0 to initialize shared libraries, must be at seg:0 */
_init:
    ret

/* normal user space entry point, so called _main() */
_main:
    /* get IRQ Routing Table */
    movq    $TEXT_ADDRESS + __PAGESIZE, %rsi
    lodsq
    shlq    $3, %rax
    xorq    %rdx, %rdx
    /* enable IRQs */
1:  cmp     $0, (%rsi)
    jz      2f
    pushq   %rax
    pushq   %rdx
    pushq   %rsi
    xorq    %rdi, %rdi
    xorq    %rsi, %rsi
    movb    $SYS_ack, %sil
    movl     $0x646E6573, %eax # 'send'
    syscall
    popq    %rsi
    popq    %rdx
    popq    %rax
2:  addq    %rax, %rsi
    incb    %dl
    cmpb    $ISR_NUMIRQ, %dl
    jne     1b
    /* release interrupts */
    xorq    %rdi, %rdi
    xorq    %rsi, %rsi
    movl    $0xB0070E01, %edx
    movb    $SYS_ack, %sil
    xchg    %bx, %bx
    movl     $0x646E6573, %eax # 'send'
    syscall

    /* get work to do */
getwork:
    call    mq_recv
    /* MSG_FUNC(msg.evt) == SYS_swapbuf? */
    cmpw    $SYS_swapbuf, (%rax)
    jne     1f
    movq    $TEXT_ADDRESS + __PAGESIZE + (ISR_NUMIRQ*ISR_NUMHANDLER+1)*8, %rsi
    /* get IRQ Routing Table */
    movq    $TEXT_ADDRESS + __PAGESIZE, %rsi
    /* rax = irt[ISR_NUMIRQ*irt[0]+1] */
    xorq    %rcx, %rcx
    movb    (%rsi), %cl /* rcx = irt[0] */
    xorq    %rdx, %rdx
    movq    $ISR_NUMIRQ, %rax
    mulq    %rcx
    incq    %rax
    shlq    $3, %rax
    addq    %rsi, %rax
    /* call mem2vid() */
    cmpq    $TEXT_ADDRESS, (%rax)
    jbe     getwork
    call    *(%rax)
    jmp     getwork

    /* MSG_FUNC(msg.evt) == SYS_IRQ? */
1:  cmpw    $SYS_IRQ, (%rax)
    jne     getwork
    /* get IRQ Routing Table */
    movq    8(%rax), %rdi /* rdi = irq */
    movq    $TEXT_ADDRESS + __PAGESIZE, %rsi
    /* rax = &irt[irq*irt[0]+1] */
    xorq    %rcx, %rcx
    movb    (%rsi), %cl /* rcx = irt[0] */
    movq    %rax, %rbx
    xorq    %rdx, %rdx
    movq    %rdi, %rax
    mulq    %rcx
    incq    %rax
    shlq    $3, %rax
    addq    %rsi, %rax
    /* now call the irq handlers (*rax)(irq); */
1:  cmpq    $TEXT_ADDRESS, (%rax)
    jbe     2f
    pushq   %rax
    pushq   %rcx
    pushq   %rdi
    call    *(%rax)
    popq    %rdi
    popq    %rcx
    popq    %rax
2:  addq    $8, %rax
    decb    %cl
    orb     %cl, %cl
    jnz     1b
    /* notify core to reenable IRQ */
    movq    %rdi, %rdx
    xorq    %rdi, %rdi
    xorq    %rsi, %rsi
    movb    $SYS_ack, %sil
    movl     $0x646E6573, %eax # 'send'
    syscall
    /* loop forever */
    jmp     getwork
