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

.global irq1

.section .text

irq1:
    /* is there a key waiting? */
    movw    $512, %cx
    xorq    %rax, %rax
    xorq    %rdx, %rdx
1:  inb     $0x64, %al
    andb    $1, %al
    jnz     2f
    decw    %cx
    jnz     1b
    ret
    /* yes, read the scancode */
2:  inb     $0x60, %al
    movb    %al, %dl
    /* acknowledge */
    inb     $0x61, %al
    outb    %al, $0x61
    /* check extended scancode */
    cmpb    $0xE0, %dl
    jne     1f
    movb    $1, kbd_prefix(%rip)
    ret
1:  cmpb    $0xE1, %dl
    jne     1f
    movb    $2, kbd_prefix(%rip)
    ret
1:  cmpb    $0xFF, %dl
    jne     1f
    movb    $3, kbd_prefix(%rip)
    ret
1:  movb    kbd_prefix(%rip), %dh
    movb    $0, kbd_prefix(%rip)
    /* extended scancode in dx */
    xorq    %rsi, %rsi
    bt      $7, %dx
    jnc     2f
    /* keyrelease */
    cmpb    $0, kbd_pressed(%rip)
    /* spurious interrupt, release without a press, no message */
    jl      4f
    cmpb    $2, kbd_pressed(%rip)
    jg      4f
    decb    kbd_pressed(%rip)
    movw    $SYS_keyrelease, %si
    /* clear release bit so that it won't mess up scancode */
    btr     $7, %dx
    jmp     3f
2:  /* keypress */
    incb    kbd_pressed(%rip)
    movw    $SYS_keypress, %si
3:  /* send a key event to UI task */
    movq    $SRV_ui, %rdi
    xorq	%rdx, %rdx
    xorq	%rcx, %rcx
    movl     $0x646E6573, %eax # 'send'
    syscall
    ret
4:  movb    $0, kbd_pressed(%rip)
    ret

.section .data
kbd_prefix:
    .byte 0
kbd_pressed:
    .byte 0
