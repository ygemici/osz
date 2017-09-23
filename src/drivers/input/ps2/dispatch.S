/*
 * drivers/input/ps2/dispatch.S
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
 * @brief PS2 event dispatcher (we can't use drv as we serve 2 IRQs)
 */

#define _AS 1
#include <osZ.h>
#include <sys/driver.h>

.global _init
.global task_init

.section .text

/* we don't use service.o's _init in drv, yet linker needs this */
task_init:
    ret

_init:
    /* initialize */
    call    mouse_init@PLT

    /* wait for an IRQ message */
1:  call    mq_recv@PLT

    /* failsafe */
    xorq    %rdi, %rdi
    movw    (%rax), %di
    andw    $0xFFF, %di
    cmpw    $SYS_IRQ, %di
    jne     1b
    xorq    %rdi, %rdi
    movb    8(%rax), %dil
    /* irq==1? */
    cmpb    $1, %dil
    jne     2f
    call    3f
    call    irq1@PLT
    jmp     1b

    /* irq==12? */
2:  cmpb    $12, %dil
    jne     1b
    call    3f
    call    irq12@PLT
    jmp     1b

3:  /* IRQ ack to re-enable */
    pushq   %rax
    xorq    %rax, %rax
    movb    $SYS_ack, %al
    syscall
    popq    %rax
    ret
