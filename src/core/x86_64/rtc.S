/*
 * core/x86_64/rtc.S
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
 * @brief RTC timer for legacy computers
 */

#define _AS 1
#include <osZ.h>

.global rtc_init

rtc_init:
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
    /* acknowledge IRQ8 in RTC */
    movb	$0x0C, %al
    outb	%al, $0x70
    jmp		1f
1:  inb		$0x71, %al

    /* reenable NMI */
    xorb    %al, %al
    outb    %al, $0x70

    movq    $1024, tmrfreq
    movb    $8, tmrirq
    ret
