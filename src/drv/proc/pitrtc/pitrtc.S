/*
 * lib/libc/x86_64/keyboard.S
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
 * @brief PS2 keyboard driver
 */

#define _AS 1
#include <osZ.h>

#include "config.h"

.global _init
.global irq0
.global irq8
.global tmrfreq

_init:
    /* initialize PIT to max/10 interrupts per sec */
    movq    $PIT_DIVISOR, %rbx
    movb    $0b00110100, %al    //bit 7,6 = (00) channel 0
                                //bit 5,4 = (11) write LSB then MSB
                                //bit 3-1 = (010) rate generator
                                //bit 0 = (0) binary counter
    outb    %al, $0x43
    movb    %bl, %al
    outb    %al, $0x40
    mov     %bh, %al
    outb    %al, $0x40

    /* initialize RTC */

    /* disable NMI */
    inb     $0x70, %al
    btsw    $8, %ax
    outb    %al, $0x70
    /* reprogram RTC to 1024Hz */
    movb    $0xA, %ah
    movb    %ah, %al
    outb    %al, $0x70
    jmp     1f
1:  inb     $0x71, %al
    andb    $0xF0, %al
    orb     $6, %al
    xchg    %ah, %al
    outb    %al, $0x70
    jmp     1f
1:  movb    %ah, %al
    outb    %al, $0x71
    /* enable periodic IRQ8 in RTC */
/*
    movb    $0xB, %ah
    movb    %ah, %al
    outb    %al, $0x70
    jmp     1f
1:  inb     $0x71, %al
    orb     $0x40, %al
    xchgb   %ah, %al
    outb    %al, $0x70
    jmp     1f
1:  movb   %ah, %al
    outb    %al, $0x71
    jmp     1f
*/
1:
  /* reenable NMI */
    inb     $0x70, %al
    btrw    $8, %ax
    outb    %al, $0x70

    /* precalculate number of ticks in 4 secs */
    movq    %rsi, %rax
    shlq    $2, %rax
    movq    %rax, rtc_total(%rip)
    movq    %rax, rtc_cnt(%rip)
    ret

irq0:
    /* nothing to do, but we still need a stub function */
    ret

irq8:
    /* acknowledge irq8 in RTC */
    movb    $0x0C, %al
    outb    %al, $0x70
    jmp     1f
1:  inb     $0x71, %al

    /* step counter */
    decq    rtc_cnt(%rip)
    cmpq    $0, rtc_cnt(%rip)
    jnz     1f
    /* reset if reached zero */
    movq    rtc_total(%rip), %rax
    movq    %rax, rtc_cnt(%rip)

    /* read precise time from CMOS and send a SYS_stime call
     * this will compensate IRQ inaccurancy on virtual machines */

    /* TODO: read cmos time */
1:  ret

.section .data
tmrfreq:
    .quad PIT_DIVISOR
rtc_cnt:
    .quad 0
rtc_total:
    .quad 0
