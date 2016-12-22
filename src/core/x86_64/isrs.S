/*
 * core/x86_64/isrs.S - GENERATED FILE DO NOT EDIT
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
 * @brief Low level exception and Interrupt Service Routines
 */
#define _AS 1
#include "isr.h"
#include "platform.h"
#include "ccb.h"

.global isr_initgates
.global isr_exc00divzero
.global isr_irq0
.extern gdt64_tss
.extern isr_irq
.extern excabort
.extern pmm

.section .data
    .align	16
idt64:
    .word	(32+16)*16-1
    .quad	0
    .align	8
ctrl:
    .asciz "PIC"
    .align 8
nopic:
    .asciz	"%s not supported"
ioctrl:
    .asciz	"IOAPIC"

.section .text
/* store thread's context into Thread Control Block */
isr_savecontext:
    ret

/* restore thread's context from Thread Control Block */
isr_loadcontext:
    ret

isr_initgates:
/* TSS64 descriptor in GDT */
    movq	$gdt64_tss, %rbx
    movl	%esi, %eax
    andl	$0xFFFFFF, %eax
    addl	%eax, 2(%rbx)
    movq	%rsi, %rax
    shrq	$24, %rax
    addq	%rax, 7(%rbx)
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
    movq	$0x0023000800000000, %rax
    wrmsr
    /* LSTAR */
    incl	%ecx
    movq	$isr_syscall, %rax
    movq    %rax, %rdx
    shrq    $32, %rdx
    wrmsr
/* enable IRQs */
    /* PIC init */
    movb	$0x11, %al
    outb	%al, $0x20
    outb	%al, $0xA0
    movb	$0x20, %al
    outb	%al, $0x21
    movb	$0x28, %al
    outb	%al, $0xA1
    movb	$0x4, %al
    outb	%al, $0x21
    movb	$0x2, %al
    outb	%al, $0xA1
    movb	$0x1, %al
    outb	%al, $0x21
    outb	%al, $0xA1
    movb	$0xFD, %al
    outb	%al, $0x21
    movb	$0xFF, %al
    outb	%al, $0xA1
    ret

/* syscall dispatcher */
.align	16, 0x90
isr_syscall:
xchg %bx, %bx
    sysret

/* exception handler ISRs */
.align 128, 0x90
isr_exc00divzero:
    cli
    lock
    btsq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    callq	isr_savecontext
    xorq	%rdi, %rdi
    movb	$0, %dil
    callq	exc00divzero
    callq	isr_loadcontext
    lock
    btrq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    iretq

.align 128, 0x90
isr_exc01debug:
    cli
    lock
    btsq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    callq	isr_savecontext
    xorq	%rdi, %rdi
    movb	$1, %dil
    callq	exc01debug
    callq	isr_loadcontext
    lock
    btrq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    iretq

.align 128, 0x90
isr_exc02nmi:
    cli
    lock
    btsq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    callq	isr_savecontext
    xorq	%rdi, %rdi
    movb	$2, %dil
    callq	excabort
    callq	isr_loadcontext
    lock
    btrq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    iretq

.align 128, 0x90
isr_exc03chkpoint:
    cli
    lock
    btsq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    callq	isr_savecontext
    xorq	%rdi, %rdi
    movb	$3, %dil
    callq	excabort
    callq	isr_loadcontext
    lock
    btrq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    iretq

.align 128, 0x90
isr_exc04overflow:
    cli
    lock
    btsq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    callq	isr_savecontext
    xorq	%rdi, %rdi
    movb	$4, %dil
    callq	excabort
    callq	isr_loadcontext
    lock
    btrq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    iretq

.align 128, 0x90
isr_exc05bound:
    cli
    lock
    btsq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    callq	isr_savecontext
    xorq	%rdi, %rdi
    movb	$5, %dil
    callq	excabort
    callq	isr_loadcontext
    lock
    btrq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    iretq

.align 128, 0x90
isr_exc06invopcode:
    cli
    lock
    btsq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    callq	isr_savecontext
    xorq	%rdi, %rdi
    movb	$6, %dil
    callq	excabort
    callq	isr_loadcontext
    lock
    btrq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    iretq

.align 128, 0x90
isr_exc07devunavail:
    cli
    lock
    btsq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    callq	isr_savecontext
    xorq	%rdi, %rdi
    movb	$7, %dil
    callq	excabort
    callq	isr_loadcontext
    lock
    btrq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    iretq

.align 128, 0x90
isr_exc08dblfault:
    cli
    lock
    btsq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    callq	isr_savecontext
    xorq	%rdi, %rdi
    movb	$8, %dil
    callq	excabort
    callq	isr_loadcontext
    lock
    btrq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    iretq

.align 128, 0x90
isr_exc09coproc:
    cli
    lock
    btsq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    callq	isr_savecontext
    xorq	%rdi, %rdi
    movb	$9, %dil
    callq	excabort
    callq	isr_loadcontext
    lock
    btrq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    iretq

.align 128, 0x90
isr_exc10invtss:
    cli
    lock
    btsq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    callq	isr_savecontext
    xorq	%rdi, %rdi
    movb	$10, %dil
    callq	excabort
    callq	isr_loadcontext
    lock
    btrq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    iretq

.align 128, 0x90
isr_exc11segfault:
    cli
    lock
    btsq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    callq	isr_savecontext
    xorq	%rdi, %rdi
    movb	$11, %dil
    callq	excabort
    callq	isr_loadcontext
    lock
    btrq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    iretq

.align 128, 0x90
isr_exc12stackfault:
    cli
    lock
    btsq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    callq	isr_savecontext
    xorq	%rdi, %rdi
    movb	$12, %dil
    callq	excabort
    callq	isr_loadcontext
    lock
    btrq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    iretq

.align 128, 0x90
isr_exc13genprot:
    cli
    lock
    btsq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    callq	isr_savecontext
    xorq	%rdi, %rdi
    movb	$13, %dil
    callq	exc13genprot
    callq	isr_loadcontext
    lock
    btrq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    iretq

.align 128, 0x90
isr_exc14pagefault:
    cli
    lock
    btsq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    callq	isr_savecontext
    xorq	%rdi, %rdi
    movb	$14, %dil
    callq	excabort
    callq	isr_loadcontext
    lock
    btrq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    iretq

.align 128, 0x90
isr_exc15unknown:
    cli
    lock
    btsq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    callq	isr_savecontext
    xorq	%rdi, %rdi
    movb	$15, %dil
    callq	excabort
    callq	isr_loadcontext
    lock
    btrq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    iretq

.align 128, 0x90
isr_exc16float:
    cli
    lock
    btsq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    callq	isr_savecontext
    xorq	%rdi, %rdi
    movb	$16, %dil
    callq	excabort
    callq	isr_loadcontext
    lock
    btrq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    iretq

.align 128, 0x90
isr_exc17alignment:
    cli
    lock
    btsq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    callq	isr_savecontext
    xorq	%rdi, %rdi
    movb	$17, %dil
    callq	excabort
    callq	isr_loadcontext
    lock
    btrq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    iretq

.align 128, 0x90
isr_exc18machinecheck:
    cli
    lock
    btsq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    callq	isr_savecontext
    xorq	%rdi, %rdi
    movb	$18, %dil
    callq	excabort
    callq	isr_loadcontext
    lock
    btrq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    iretq

.align 128, 0x90
isr_exc19double:
    cli
    lock
    btsq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    callq	isr_savecontext
    xorq	%rdi, %rdi
    movb	$19, %dil
    callq	excabort
    callq	isr_loadcontext
    lock
    btrq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    iretq

/* IRQ handler ISRs */
.align 128, 0x90
isr_irq0:
    /* preemption timer */
    cli
    lock
    btsq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    call	isr_savecontext
    xorq	%rdi, %rdi
    call	isr_irq
    /* PIC EOI */
    movb	$0x20, %al
    outb	%al, $0x20
    call	isr_loadcontext
    lock
    btrq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    iretq

.align 128, 0x90
isr_irq1:
    cli
    lock
    btsq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    call	isr_savecontext
    xorq	%rdi, %rdi
    movb	$1, %dil
    call	isr_irq
    /* PIC EOI */
    movb	$0x20, %al
    outb	%al, $0x20
    call	isr_loadcontext
    lock
    btrq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    iretq

.align 128, 0x90
isr_irq2:
    cli
    lock
    btsq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    call	isr_savecontext
    xorq	%rdi, %rdi
    movb	$2, %dil
    call	isr_irq
    /* PIC EOI */
    movb	$0x20, %al
    outb	%al, $0x20
    call	isr_loadcontext
    lock
    btrq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    iretq

.align 128, 0x90
isr_irq3:
    cli
    lock
    btsq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    call	isr_savecontext
    xorq	%rdi, %rdi
    movb	$3, %dil
    call	isr_irq
    /* PIC EOI */
    movb	$0x20, %al
    outb	%al, $0x20
    call	isr_loadcontext
    lock
    btrq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    iretq

.align 128, 0x90
isr_irq4:
    cli
    lock
    btsq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    call	isr_savecontext
    xorq	%rdi, %rdi
    movb	$4, %dil
    call	isr_irq
    /* PIC EOI */
    movb	$0x20, %al
    outb	%al, $0x20
    call	isr_loadcontext
    lock
    btrq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    iretq

.align 128, 0x90
isr_irq5:
    cli
    lock
    btsq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    call	isr_savecontext
    xorq	%rdi, %rdi
    movb	$5, %dil
    call	isr_irq
    /* PIC EOI */
    movb	$0x20, %al
    outb	%al, $0x20
    call	isr_loadcontext
    lock
    btrq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    iretq

.align 128, 0x90
isr_irq6:
    cli
    lock
    btsq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    call	isr_savecontext
    xorq	%rdi, %rdi
    movb	$6, %dil
    call	isr_irq
    /* PIC EOI */
    movb	$0x20, %al
    outb	%al, $0x20
    call	isr_loadcontext
    lock
    btrq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    iretq

.align 128, 0x90
isr_irq7:
    cli
    lock
    btsq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    call	isr_savecontext
    xorq	%rdi, %rdi
    movb	$7, %dil
    call	isr_irq
    /* PIC EOI */
    movb	$0x20, %al
    outb	%al, $0x20
    call	isr_loadcontext
    lock
    btrq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    iretq

.align 128, 0x90
isr_irq8:
    cli
    lock
    btsq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    call	isr_savecontext
    xorq	%rdi, %rdi
    movb	$8, %dil
    call	isr_irq
    /* PIC EOI */
    movb	$0x20, %al
    outb	%al, $0xA0
    outb	%al, $0x20
    call	isr_loadcontext
    lock
    btrq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    iretq

.align 128, 0x90
isr_irq9:
    cli
    lock
    btsq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    call	isr_savecontext
    xorq	%rdi, %rdi
    movb	$9, %dil
    call	isr_irq
    /* PIC EOI */
    movb	$0x20, %al
    outb	%al, $0xA0
    outb	%al, $0x20
    call	isr_loadcontext
    lock
    btrq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    iretq

.align 128, 0x90
isr_irq10:
    cli
    lock
    btsq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    call	isr_savecontext
    xorq	%rdi, %rdi
    movb	$10, %dil
    call	isr_irq
    /* PIC EOI */
    movb	$0x20, %al
    outb	%al, $0xA0
    outb	%al, $0x20
    call	isr_loadcontext
    lock
    btrq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    iretq

.align 128, 0x90
isr_irq11:
    cli
    lock
    btsq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    call	isr_savecontext
    xorq	%rdi, %rdi
    movb	$11, %dil
    call	isr_irq
    /* PIC EOI */
    movb	$0x20, %al
    outb	%al, $0xA0
    outb	%al, $0x20
    call	isr_loadcontext
    lock
    btrq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    iretq

.align 128, 0x90
isr_irq12:
    cli
    lock
    btsq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    call	isr_savecontext
    xorq	%rdi, %rdi
    movb	$12, %dil
    call	isr_irq
    /* PIC EOI */
    movb	$0x20, %al
    outb	%al, $0xA0
    outb	%al, $0x20
    call	isr_loadcontext
    lock
    btrq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    iretq

.align 128, 0x90
isr_irq13:
    cli
    lock
    btsq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    call	isr_savecontext
    xorq	%rdi, %rdi
    movb	$13, %dil
    call	isr_irq
    /* PIC EOI */
    movb	$0x20, %al
    outb	%al, $0xA0
    outb	%al, $0x20
    call	isr_loadcontext
    lock
    btrq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    iretq

.align 128, 0x90
isr_irq14:
    cli
    lock
    btsq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    call	isr_savecontext
    xorq	%rdi, %rdi
    movb	$14, %dil
    call	isr_irq
    /* PIC EOI */
    movb	$0x20, %al
    outb	%al, $0xA0
    outb	%al, $0x20
    call	isr_loadcontext
    lock
    btrq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    iretq

.align 128, 0x90
isr_irq15:
    cli
    lock
    btsq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    call	isr_savecontext
    xorq	%rdi, %rdi
    movb	$15, %dil
    call	isr_irq
    /* PIC EOI */
    movb	$0x20, %al
    outb	%al, $0xA0
    outb	%al, $0x20
    call	isr_loadcontext
    lock
    btrq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    iretq

.align 128, 0x90
isr_irq16:
    cli
    lock
    btsq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    call	isr_savecontext
    xorq	%rdi, %rdi
    movb	$16, %dil
    call	isr_irq
    /* PIC EOI */
    movb	$0x20, %al
    outb	%al, $0xA0
    outb	%al, $0x20
    call	isr_loadcontext
    lock
    btrq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    iretq

