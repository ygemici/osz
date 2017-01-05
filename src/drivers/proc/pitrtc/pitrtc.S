/*
 * drivers/proc/pitrtc.S
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
 * @brief PIT and RTC timers for legacy computers
 */

#define _AS 1
#include <osZ.h>

#include "config.h"

.global _init
.global irq0
/*.global irq8*/
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

    /* reprogram RTC to 1024Hz */
    movb    $0x8A, %ah
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
    movb    $0x8B, %ah
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
1:
    /* rtc counter to update system time */
    movq    $1024, %rax
    movq    %rax, rtc_total(%rip)
    movq    %rax, rtc_cnt(%rip)

  /* reenable NMI */
    xorb    %al, %al
    outb    %al, $0x70
    ret

irq0:
    /* nothing to do, but we still need a stub function */
    ret

sirq8:
    /* acknowledge irq8 in RTC */
    movb    $0x0C, %al
    outb    %al, $0x70
    jmp     1f
1:  inb     $0x71, %al

    /* step counter */
    decq    rtc_cnt(%rip)
    cmpq    $0, rtc_cnt(%rip)
    jnz     3f
    /* reset if reached zero */
    movq    rtc_total(%rip), %rax
    movq    %rax, rtc_cnt(%rip)

    /* read precise time from CMOS and send a SYS_stime call
     * this will compensate IRQ inaccurancy on virtual machines */

    /* read cmos time */
    lea     cmos(%rip), %rbx

    /* wait for update finish, when register A bit 7 turns off */
/*
2:  movb    $0x8A, %al
    outb    %al, $0x70
    jmp     1f
1:  inb     $0x71, %al
    btw     $8, %ax
    jnc     2b
2:  movb    $0x8A, %al
    outb    %al, $0x70
    jmp     1f
1:  inb     $0x71, %al
    btw     $8, %ax
    jc      2b
*/
    /* ok, now we can read date time */

    /* century */
    movb    $0xB2, %al
    outb    %al, $0x70
    jmp     1f
1:  inb     $0x71, %al
    orb     %al, %al
    jnz     2f
    movb    $0x20, %al
2:  movb    %al, 0(%rbx)
    /* year */
    movb    $0x89, %al
    outb    %al, $0x70
    jmp     1f
1:  inb     $0x71, %al
    movb    %al, 1(%rbx)
    /* month */
    movb    $0x88, %al
    outb    %al, $0x70
    jmp     1f
1:  inb     $0x71, %al
    movb    %al, 2(%rbx)
    /* day */
    movb    $0x87, %al
    outb    %al, $0x70
    jmp     1f
1:  inb     $0x71, %al
    movb    %al, 3(%rbx)
    /* hours */
    movb    $0x84, %al
    outb    %al, $0x70
    jmp     1f
1:  inb     $0x71, %al
    movb    %al, 4(%rbx)
    /* min */
    movb    $0x82, %al
    outb    %al, $0x70
    jmp     1f
1:  inb     $0x71, %al
    movb    %al, 5(%rbx)
    /* sec */
    movb    $0x80, %al
    outb    %al, $0x70
    jmp     1f
1:  inb     $0x71, %al
    movb    %al, 6(%rbx)

    xorq    %rdi, %rdi
    movb    $SYS_stimebcd, %dil
    movq    (%rbx), %rsi
    movl     $0x646E6573, %eax # 'send'
    syscall

3:  /* reenable NMI */
    xorb    %al, %al
    outb    %al, $0x70
    ret

.section .data
tmrfreq:
    .quad PIT_DIVISOR
rtc_cnt:
    .quad 0
rtc_total:
    .quad 0
cmos:
    .quad 0
