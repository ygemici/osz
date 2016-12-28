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
.global irq8

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

    /* initialize PIT */
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
1:  /* reenable NMI */
    inb     $0x70, %al
    btrw    $8, %ax
    outb    %al, $0x70

/*
    # RTC enable
    inb     $0xA1, %al
    andb    $0xFF-1, %al
    outb    %al, $0xA1
    # PIC enable
    inb     $0x21, %al
    andb    $0xFF-5, %al
    outb    %al, $0x21
*/

    ret

irq0:
    xorb    %al, %al
    outb    %al, $0x43
    inb     $0x40, %al
    jmp     1f
1:  inb     $0x40, %al
/*
    # PIC EOI
    movb    $0x20, %al
    outb    %al, $0x21
    inb     $0x21, %al
    andb    $0xFF-5, %al
    outb    %al, $0x21
*/
    ret

irq8:
    ret
