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
    movl    %eax, %ds
    movl    %eax, %es
    movl    %eax, %ss
    movl    $0x18, %eax
    movl    %eax, %fs
    movl    %eax, %gs
/* save core mappings pointer in PML4 */
    movq    %cr3, %rax
    addq    $4096-8, %rax
    movq    (%rax), %rax
    movq    %rax, core_mapping
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
