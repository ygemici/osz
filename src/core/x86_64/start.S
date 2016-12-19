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

.section .text
.global _start
.global cpu_init
.global gdt64_tss
.extern main
.extern core_mapping

/**********************************************************************
 *                     OS/Z Loader entry point                        *
 **********************************************************************

    Registers
        rax     magic 'BOOTBOOT' (0x544f4f42544f4f42)
        rbx     virtual address of bootparams
        rcx     virtual address of environment
        rdx     virtual address of framebuffer
*/
_start:
    cli
    cld
/* setup segments */
    movq    $gdt64, %rax
    lgdt    (%rax)
    jmp     1f
    nop
1:  movl    $0x10, %eax
    movl    %eax, %es
    movl    %eax, %ss
    movl    $0x1b, %eax
    movl    %eax, %ds
    movl    %eax, %es
    movl    %eax, %fs
    movl    %eax, %gs
/* setup stack (should be ok, but be sure) */
    xor     %rsp, %rsp
/* jump to 64 bit code segment */
    pushq   $0x08
    pushq   $main
    lretq

    .align  16
gdt64:
    .word   gdt64_end-gdt64_start-1
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
gdt64_end:

cpu_init:
    xorq	%rax, %rax
    incb    %al
    cpuid
    /* minimum processor family */
    movq    $0x796C696D6166, %rsi
    cmp     $0x0600, %ax
    jb      1f
    /* SSE3 */
    movq    $0x33455353, %rsi
    bt      $0, %ecx
    jnc     1f
    /* FXSR */
    movq    $0x52535846, %rsi
    bt      $24, %edx
    jnc     1f
    /* PSE */
    movq    $0x00455350, %rsi
    bt      $3, %edx
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

    movl    $0x80000001, %eax
    cpuid
    /* NX */
    movq    $0x0000584E, %rsi
    bt      $20, %edx
    jnc     1f
    /* SCALL */
    movq    $0x4C4C4143535953, %rsi
    bt      $11, %edx
    jnc     1f

    /* enable FXSAFE and NX */
    movl    $0xC0000080, %ecx
    rdmsr
    orl     $0x2400, %eax
/*    wrmsr*/

    /* enable OSFXSR and TSC */
    movq    %cr4, %rax
    orl    $0x102, %eax
    movq    %rax, %cr4
    ret

    /* feature not supported */
1:  movq    $nocpu, %rdi
    call    kpanic
nocpu:
.asciz "CPU feature %a not supported"
