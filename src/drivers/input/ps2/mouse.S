/*
 * drivers/input/ps2/mouse.S
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
 * @brief PS2 mouse driver
 */

#define _AS 1
#include <osZ.h>

.global _init
.global irq12

.section .text

_init:
    /* initialize legacy ps2 user input */
    xorq    %rax, %rax
    movb    $2, %dl
    call    ps2wait
    movb    $0xA8, %al
    outb    %al, $0x64
    /* get ack */
    call    ps2rd
    /* some voodoo magic to enable irq12 */
    movb    $2, %dl
    call    ps2wait
    movb    $0x20, %al
    outb    %al, $0x64
    movb    $1, %dl
    call    ps2wait
    inb     $0x60, %al
    bts     $1, %ax
    btr     $5, %ax
    movb    %al, %bl
    movb    $2, %dl
    call    ps2wait
    movb    $0x60, %al
    outb    %al, $0x64
    call    ps2wait
    movb    %bl, %al
    outb    %al, $0x60
    /* get optional ack */
    movb    $1, %dl
    call    ps2wait
    ret

irq12:
    ret

    /* dl=1 read, dl=2 write */
ps2wait:
    /* is there an input waiting? */
    movw    $512, %cx
1:  inb     $0x64, %al
    andb    %dl, %al
    jnz     1f
    decw    %cx
    jnz     1b
1:  ret

ps2wr:
    movb    %al, %dh
    movb    $2, %dl
    call    ps2wait
    movb    $0xD4, %al
    outb    %al, $0x64
    call    ps2wait
    movb    %dh, %al
    outb    %al, $0x60
    /*no ret, fall into read code to read ack */
ps2rd:
    mov     $1, %dl
    call    ps2wait
    inb     $0x60, %al
    ret
