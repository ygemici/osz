/*
 * drivers/input/ps2/mouse.S
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
 * @brief PS2 mouse driver
 */

#define _AS 1
#include <osZ.h>

.global mouse_init
.global irq12
.type irq12, STT_FUNC
.global _packet

.section .text

mouse_init:
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
    /* restore to defaults */
    movb    $0xF6, %al
    call    ps2wr
    movb    $3, cntmax(%rip)
    cmpb    $0, nozaxis(%rip)
    jnz     .nozaxis
    /* enable Z axis */
    movb    $0xF3, %al
    call    ps2wr
    movb    $200, %al
    call    ps2wr
    movb    $0xF3, %al
    call    ps2wr
    movb    $100, %al
    call    ps2wr
    movb    $0xF3, %al
    call    ps2wr
    movb    $80, %al
    call    ps2wr
    movb    $0xF2, %al
    call    ps2wr
    call    ps2rd
    or      %al, %al
    jz      .nozaxis
    movb    $4, cntmax(%rip)
.nozaxis:
    /* enable 4th and 5th buttons */
    movb    $0xF3, %al
    call    ps2wr
    movb    $200, %al
    call    ps2wr
    movb    $0xF3, %al
    call    ps2wr
    movb    $200, %al
    call    ps2wr
    movb    $0xF3, %al
    call    ps2wr
    movb    $80, %al
    call    ps2wr
    movb    $0xF2, %al
    call    ps2wr
    call    ps2rd
    /* set sample rate */
    movb    $0xF3, %al
    call    ps2wr
    movb    samplerate(%rip), %al
    call    ps2wr
    /* set resolution */
    movb    $0xE8, %al
    call    ps2wr
    movb    resolution(%rip), %al
    call    ps2wr
    /* set scaling 1:1 */
    movb    $0xE6, %al
    call    ps2wr
    /* enable pointer */
    movb    $0xF4, %al
    call    ps2wr
    /* reset variables */
    movb    $0, cnt(%rip)
    xorq    %rax, %rax
    movq    %rax, packet(%rip)
    ret

irq12:
    /* read next byte in packet */
    xorq    %rax, %rax
    xorq    %rcx, %rcx
    xorq    %rdx, %rdx
    movw    $1000, %cx
1:  inb     $0x64, %al
    decw    %cx
    jz      .end
    andb    $0x20, %al
    jz      1b
    inb     $0x60, %al
    movb    %al, %dl
    inb     $0x61, %al
    outb    %al, $0x61
    lea     packet(%rip), %rbx
    addb    cnt(%rip), %bl
    movb    %dl, (%rbx)
    incb    cnt(%rip)
    movb    cnt(%rip), %al
   /* do we have all bytes? */
    cmpb    cntmax(%rip), %al
    jb      .end
    movb    $0, cnt(%rip)
    movq    packet(%rip), %rax
    /* get buttons */
    xorq    %rsi, %rsi
    movb    %al, %sil
    shrq    $8, %rax
    movq    packet(%rip), %rsi
    /* get delta X */
    movsxb  %al, %rdx
    shrq    $8, %rax
    /* get delta Y */
    movsxb  %al, %r10
    shrq    $8, %rax
    /* get delta Z */
    movsxb  %al, %r8
    /* reset packet */
    xorq    %rax, %rax
    movq    %rax, packet(%rip)
    /* send a key event to UI task */
    xorq    %r9, %r9
    movq    $SRV_UI, %rdi
    shlq    $16, %rdi
    movw    $SYS_pointer, %di
    movl     $0x646E6573, %eax # 'send'
    syscall
.end:
    ret

    /* dl=1 read, dl=2 write */
ps2wait:
    /* is there an input waiting? */
    movw    $1000, %cx
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

.section .data
nozaxis:
    .byte 0
samplerate:
    .byte 200
resolution:
    .byte 3
cnt:
    .byte 0
cntmax:
    .byte 0
    .align 8
_packet:
packet:
    .quad 0

