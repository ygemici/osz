/*
 * core/x86_64/ibmpc/waitkey.S
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
 * @brief low level kernel routines for the core
 */

#define _AS 1
#include "../arch.h"

.section .text
.global platform_srand
.type platform_srand, STT_FUNC
.global platform_waitkey
.type platform_waitkey, STT_FUNC

/* initialize random seed */
platform_srand:
    movq    $srand, %rdi
    movq    $CORE_ADDRESS, %rsi
    movw    $512, %cx
1:  lodsq
    addq    %rax, (%rdi)
    subq    %rax, 8(%rdi)
    xorq    %rax, 16(%rdi)
    addq    %rax, 24(%rdi)
    dec     %cx
    jnz     1b
    call    kentropy
    ret

/* an early implementation, called by kprintf */
platform_waitkey:
    xorq    %rax, %rax
    xorq    %rdx, %rdx
    /* is there a key waiting? */
1:  inb     $0x64, %al
    andb    $1, %al
    jnz     2f
    pause
#if DEBUG
    movw    $0x3fd, %dx
    inb     %dx, %al
    /* do we have a serial port at all? */
    cmpb    $0xFF, %al
    je      1b
    /* is there a received byte waiting? */
    andb    $1, %al
    jz      1b
    subb    $5, %dl
    inb     %dx, %al
    orb     %al, %al
    jz      1b
    movb    $0, dbg_iskbd
    jmp     8f
#else
    jmp     1b
#endif
    /* yes, read the scancode */
2:  inb     $0x60, %al
    orb     %al, %al
    jz      1b
    movb    %al, %dl
    /* acknowledge */
    inb     $0x61, %al
    outb    %al, $0x61
#if DEBUG
    /* don't accept Left Alt key. I press it when I press Alt+Q or Alt+F4 to close qemu and bochs :-) */
    cmpb    $0x38, %dl
    je      1b
    /* handle shift key here for debugger */
    movb    $1, dbg_iskbd
    cmpb    $54, %dl
    je      2f
    cmpb    $42, %dl
    jne     1f
2:  movb    $1, dbg_isshft
    jmp     3f
1:  cmpb    $54+0x80, %dl
    je      2f
    cmpb    $42+0x80, %dl
    jne     3f
2:  movb    $0, dbg_isshft
3:
#endif
    /* check extended scancode */
    cmpb    $0xE0, %dl
    jne     1f
    movb    $1, 9f
    jmp     platform_waitkey
1:  cmpb    $0xE1, %dl
    jne     1f
    movb    $2, 9f
    jmp     platform_waitkey
1:  cmpb    $0xFF, %dl
    jne     1f
    movb    $3, 9f
    jmp     platform_waitkey
1:  xorq    %rax, %rax
    movb    9f, %ah
    movb    $0, 9f
    movb    %dl, %al
    bt      $7, %ax
    jc      platform_waitkey
8:  /* extended scancode in ax */
    ret
9:
    .byte 0
