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

/* normal entry point, so called main() */
_main:
    /* get work to do */
    call    clrecv
    /* MSG_FUNC(msg.evt) != SYS_IRQ? */
    cmpw    $SYS_IRQ, (%rax)
    jne     _main
    /* get IRQ Routing Table */
    movq    $TEXT_ADDRESS + __PAGESIZE, %rsi
    /* rax = &irt[(irq-1)*irt[0]+1] */
    xorq    %rcx, %rcx
    movb    (%rsi), %cl
    movq    %rax, %rbx
    xorq    %rax, %rax
    xorq    %rdx, %rdx
    movb    8(%rbx), %al
    movq    %rax, %rdi
    decb    %al
    mulq    %rcx
    incq    %rax
    shlq    $3, %rax
    addq    %rsi, %rax
    /* now call the irq handlers (*rax)(irq); */
1:  cmpq    $TEXT_ADDRESS, (%rax)
    jbe     _main
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
    jmp     _main
