.section .text
.global _start
.extern main

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
/* setup segments */
	movq	$gdt64, %rax
	lgdt	(%rax)
	jmp		1f
	nop
1:	movl	$0x10, %eax
	movl	%eax, %ds
	movl	%eax, %es
	movl	%eax, %ss
	movl	$0x20, %eax
	movl	%eax, %fs
	movl	%eax, %gs
/* setup task register */
/* ltr */
/* setup stack (should be ok) */
	xor		%rsp, %rsp
	pushq	$0x8
	pushq	$main
	lretq

	.align	16
gdt64:
	.word	gdt64_end-gdt64_start-1
	.quad	gdt64_start
	.align	64

gdt64_start:
	.quad 0x0000000000000000	/* always empty */
	.quad 0x002098000000ffff	/* 08 core CS */
	.quad 0x008092000000ffff	/* 10 core DS */
	.quad 0x0020f8000000ffff	/* 18 user CS */
	.quad 0x0080f2000000ffff	/* 20 user DS */
	.quad 0x0400890302010068	/* 28 tss */
gdt64_end:
