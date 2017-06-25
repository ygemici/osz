/*
 * core/x86_64/start.S
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
 * @brief Kernel entry point
 */

#include "isr.h"

.section .text
.global _start
.global gdt64_tss
.extern main

/**********************************************************************
 *                     OS/Z Loader entry point                        *
 **********************************************************************

    Registers
        rax     magic 'BOOTBOOT' (0x544f4f42544f4f42)
        rbx     virtual address of bootboot stucture
        rcx     virtual address of environment
        rdx     virtual address of framebuffer
*/
_start:
    cli
    cld
/* disable PIC and NMI */
    movb	$0xFF, %al
    outb	%al, $PIC_MASTER_DATA
    outb	%al, $PIC_SLAVE_DATA
    inb		$0x70, %al
    btsw	$8, %ax
    outb	%al, $0x70
#if DEBUG
/* clear debug status */
    xorq    %rax, %rax
    movq    %rax, %dr6
#endif
/* setup segments */
    movq    $gdt64, %rax
    lgdt    (%rax)
    jmp     1f
    nop
1:  movl    $0x1b, %eax
    movl    %eax, %ds
    movl    %eax, %es
    movl    %eax, %fs
    movl    %eax, %gs
    movl    $0x10, %eax
    movl    %eax, %ss
/* setup stack frame */
    xorq    %rsp, %rsp
    xorq    %rbp, %rbp
    /* initialize console so that we can report errors and stats */
    callq   kprintf_init
    /* clear task rip to avoid symbol lookup in case of panic */
    xorq    %rax, %rax
    movq    %rax, __PAGESIZE-40

    /* init entropy */
    movq    $srand, %rdi
    movq    %rbx, %rsi
    movw    $512, %cx
1:  lodsq
    addq    %rax, (%rdi)
    subq    %rax, 8(%rdi)
    xorq    %rax, 16(%rdi)
    addq    %rax, 24(%rdi)
    pushq   %rcx
    callq   kentropy
    popq    %rcx
    dec     %cx
    jnz     1b
/* check and enable all CPU features that this release use
   and gain entropy in the meantime */
    xorq    %rax, %rax
    incb    %al
    cpuid
    xorq    %rax, (%rdi)
    xorq    %rbx, 8(%rdi)
    xorq    %rcx, 16(%rdi)
    xorq    %rdx, 24(%rdi)
    /* minimum processor family 6 */
    movq    $0x796C696D6166, %rsi
    cmp     $0x0600, %ax
    jb      1f
    /* FXSR */
    movq    $0x52535846, %rsi
    bt      $24, %edx
    jnc     1f
    /* TSC */
    movq    $0x00435354, %rsi
    bt      $4, %edx
    jnc     1f
    /* MSR */
    movq    $0x0052534D, %rsi
    bt      $5, %edx
    jnc     1f
    /* PAT */
    movq    $0x00544150, %rsi
    bt      $16, %edx
    jnc     1f
    /* SSSE3 (needed for alpha blending) */
/*
    movq    $0x3345535353, %rsi
    bt      $9, %ecx
    jnc     1f
*/
#if ISR_CTRL == CTRL_x2APIC
    /* x2APIC */
    movq    $0x434950413278, %rsi
    bt      $21, %ecx
    jnc     1f
#endif
#if OPTIMIZE == 1
    /* AVX */
    movq    $0x00585641, %rsi
    bt      $28, %ecx
    jnc     1f
#endif

    movl    $0x80000001, %eax
    cpuid
    /* NX */
    movq    $0x0000584E, %rsi
    bt      $20, %edx
    jnc     1f
    /* SYSCALL */
    movq    $0x4C4C4143535953, %rsi
    bt      $11, %edx
    jnc     1f

    /* enable NX and SYSCALL */
    movl    $0xC0000080, %ecx
    rdmsr
    orl     $0x801, %eax
    wrmsr

    /* enable OSFXSR and TSC */
    movq    %cr4, %rax
#if DEBUG
    orl     $0x208, %eax
#else
    orl     $0x200, %eax
#endif
    andb    $0xFB, %al
    movq    %rax, %cr4

    call    kentropy

/* jump to C function main() in 64 bit code segment */
    pushq   $0x08
    pushq   $main
    lretq

    /* feature not supported */
1:  movq    $nocpu, %rdi
    call    kpanic

/* placed in text intentionally, it's rodata. Yes, GDT too, we never change it. */
nocpu:
.asciz "CPU feature %a not supported"
    .align  16
gdt64:
    .word   1f-gdt64_start
    .quad   gdt64_start
    .align  64
gdt64_start:
    .quad 0x0000000000000000    /* always empty */
    .quad 0x002098000000ffff    /* 08 core CS */
    .quad 0x008092000000ffff    /* 10 core DS */
    .quad 0x0080f2000000ffff    /* 18 user DS */
    .quad 0x0020f8000000ffff    /* 20 user CS */
gdt64_tss:
    .quad 0x0000890000000068    /* 28 tss */
    .quad 0x0000000000000000
1:
