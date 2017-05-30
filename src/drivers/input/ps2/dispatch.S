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

.global _init
.extern mouse_init

.section .text

_init:
    /* initialize */
    call    mouse_init@PLT
    /* wait for an IRQ message */
1:  call    mq_recv@PLT
    /* failsafe */
    cmpw    $SYS_IRQ, (%rax)
    jne     1b
    /* irq==1? */
    cmpw    $1, 8(%rax)
    je      2f
    /* irq==12? */
    cmpw    $12, 8(%rax)
    jne     1b
    call    irq12@PLT
    /* IRQ ack to re-enable */
    movq    $12, %rsi
    jmp     3f
2:  call    irq1@PLT
    /* IRQ ack to re-enable */
    movq    $1, %rsi
3:  xorq    %rdi, %rdi
    movb    $SYS_ack, %dil
    movl     $0x646E6573, %eax # 'send'
    syscall
    jmp     1b
