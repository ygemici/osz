/*
 * drivers/proc/pit.S
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
 * @brief PIT timer for legacy computers
 */

#define _AS 1
#include <osZ.h>

.global _init
.global irq0
.global tmrfreq

_init:
    /* initialize PIT */
    // 1193180/freq
    movq    $1193, %rbx
    movb    $0b00110100, %al    //bit 7,6 = (00) channel 0
                                //bit 5,4 = (11) write LSB then MSB
                                //bit 3-1 = (010) rate generator
                                //bit 0 = (0) binary counter
    outb    %al, $0x43
    movb    %bl, %al
    outb    %al, $0x40
    mov     %bh, %al
    outb    %al, $0x40

    ret

irq0:
    /* nothing to do, but we still need a stub function 
       to inform SYS task we need IRQ enabled */
    ret

.section .data
tmrfreq:
    .quad 1000
