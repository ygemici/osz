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
    xorq    %rdx, %rdx
    /* enable IRQs */
1:  cmp     $0, (%rsi)
    jz      2f
    pushq   %rdx
    pushq   %rsi
    xorq    %rdi, %rdi
    xorq    %rsi, %rsi
    movb    $SYS_ack, %sil
    movl     $0x646E6573, %eax # 'send'
    syscall
    popq    %rsi
    popq    %rdx
2:  addq    %rax, %rsi
    incb    %dl
    cmpb    $ISR_NUMIRQ, %dl
    jne     1b

    /* get work to do */
getwork:
    call    mq_recv
    /* MSG_FUNC(msg.evt) != SYS_IRQ? */
    cmpw    $SYS_IRQ, (%rax)
    jne     getwork
    /* get IRQ Routing Table */
    movq    $TEXT_ADDRESS + __PAGESIZE, %rsi
    /* rax = &irt[irq*irt[0]+1] */
    xorq    %rcx, %rcx
    movb    (%rsi), %cl
    movq    %rax, %rbx
    xorq    %rax, %rax
    xorq    %rdx, %rdx
    movb    8(%rbx), %al
    movq    %rdi, %rax
    mulq    %rcx
    incq    %rax
    shlq    $3, %rax
    addq    %rsi, %rax
    /* now call the irq handlers (*rax)(irq); */
1:  cmpq    $TEXT_ADDRESS, (%rax)
    jbe     getwork
    pushq   %rax
    pushq   %rcx
    pushq   %rdi
    call    *(%rax)
    popq    %rdi
    popq    %rcx
    popq    %rax
    addq    $8, %rax
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
