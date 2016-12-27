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
#include <syscall.h>

.global _init
.global irq0
/*.global irq8*/

_init:
    xorq    %rdi, %rdi
    movq    $SYS_sysinfo, %rsi
    movl     $0x646E6573, %eax # 'send'
    syscall
    movq    $4096, %rbx
    movq     8(%rbx), %rax
    /* must match sizeof(msg_t) */
    shlq    $6, %rax
    addq    %rbx, %rax
    /* mqhdr->mq_end++ */
    incq    8(%rbx)
    movq    8(%rbx), %rcx
    cmpq    16(%rbx), %rcx
    jne     1f
    movq    $1, 8(%rbx)
1:
    mov     8(%rax), %rax
    /*rbx=quantum*/
    movq    (%rax), %rbx

    movq    $1193180, %rax
    xorq    %rdx, %rdx
    divl    %ebx
    xchgl   %eax, %ebx
    movb    $0b00110100, %al    //bit 7,6 = (00) channel 0
                                //bit 5,4 = (11) write LSB then MSB
                                //bit 3-1 = (010) rate generator
                                //bit 0 = (0) binary counter
    outb    %al, $0x43
    movb    %bl, %al
    outb    %al, $0x40
    jmp     2f
2:  mov     %bh, %al
    outb    %al, $0x40
    inb     $0x21, %al
    btr     $0, %ax
    btr     $1, %ax
    outb    %al, $0x21
    ret

irq0:
    movb    $0x20, %al
    outb    %al, $0x21
    ret

.irq8:
    ret
