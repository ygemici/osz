/*
 * x86_64/isrs.S
 * 
 * Copyright 2016 CC-by-nc-sa-4.0 bztsrc@github
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
 * @brief Low level Interrupt Service Routines
 */
.include "isr.h"

.section .text
.global isr_initgates
.global isr_syscall
.global isr_exc00
.extern gdt64_tss

isr_initgates:
/* TSS64 descriptor in GDT */
	movq	$gdt64_tss, %rbx
	movl	%edi, %eax
	andl	$0xFFFFFF, %eax
	addl	%eax, 2(%rbx)
	movq	%rsi, %rax
	shlq	$24, %rax
	addl	%eax, 7(%rbx)
/* setup task register */
	movl	$0x28 + 3, %eax
	ltr		%ax
/* IDTR */
	movq	$idt64, %rax
	movq	%rdi, 2(%rax)
	lidt	(%rax)
/* setup syscall dispatcher */
	/* STAR */
	xorq	%rcx, %rcx
	movl	$0xC0000081, %ecx
	movq	$0x0013000800000000, %rax
	wrmsr
	/* LSTAR */
	inc		%ecx
	xorl	%edx, %edx
	movq	$isr_syscall, %rax
	wrmsr
	ret
.section .data
	.align	16
idt64:
	.word	32*16-1
	.quad	0
	.align	64

.section .text

/* syscall dispatcher */
.align	16
isr_syscall:
	sysret

/* exception handler ISRs */
.align	128
isr_exc00:
	iretq

.align	128
/* IRQ handler ISRs */
isr_irq00:
	iretq
