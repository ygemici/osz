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
#include <errno.h>
#include "isr.h"
#include "platform.h"
#include "ccb.h"
#include "tcb.h"

.global isr_initgates
.global isr_exc00divzero
.global isr_irq0
.global isr_initirq
.global isr_enableirq
.global isr_disableirq
.global isr_gainentropy
.extern isr_ticks
.extern isr_entropy
.extern gdt64_tss
.extern excabort
.extern pmm

.section .data
    .align	16
idt64:
    .word	(32+16)*16-1
    .quad	0
    .align	8
isr_preemptcnt:
    .quad	0
ctrl:
    .asciz "PIC"
    .align 8
nopic:
    .asciz	"%s not supported"
ioctrl:
    .asciz	"IOAPIC"

.section .text
/* void isr_enableirq(uint64_t irq) */
isr_enableirq:
    cmpl	$2, %edi
    je		1f
    xorl	%edx, %edx
    incl	%edx
    movl	%edi, %ecx
    cmpl	$8, %ecx
    jae		2f
    shll	%cl, %edx
    notl	%edx
    inb		$PIC_MASTER_DATA, %al
    andb	%dl, %al
    outb	%al, $PIC_MASTER_DATA
1:  ret
2:  subl	$8, %edi
    shll	%cl, %edx
    notl	%edx
    inb		$PIC_SLAVE_DATA, %al
    andb	%cl, %al
    outb	%al, $PIC_SLAVE_DATA
    ret

/* void isr_disableirq(uint64_t irq) */
isr_disableirq:
    cmpl	$2, %edi
    je		1f
    xorl	%edx, %edx
    incl	%edx
    movl	%edi, %ecx
    cmpl	$8, %edi
    jae		2f
    shll	%cl, %edx
    inb		$PIC_MASTER_DATA, %al
    orb		%cl, %al
    outb	%al, $PIC_MASTER_DATA
1:  ret
2:  subl	$8, %edi
    shll	%cl, %edx
    inb		$PIC_SLAVE_DATA, %al
    orb		%cl, %al
    outb	%al, $PIC_SLAVE_DATA
    ret

/* store thread's context into Thread Control Block */
isr_savecontext:
    movq	%rax, OSZ_tcb_gpr +   0
    movq	%rbx, OSZ_tcb_gpr +   8
    movq	%rcx, OSZ_tcb_gpr +  16
    movq	%rdx, OSZ_tcb_gpr +  24
    movq	%rsi, OSZ_tcb_gpr +  32
    movq	%rdi, OSZ_tcb_gpr +  40
    movq	%r8,  OSZ_tcb_gpr +  48
    movq	%r9,  OSZ_tcb_gpr +  56
    movq	%r10, OSZ_tcb_gpr +  64
    movq	%r11, OSZ_tcb_gpr +  72
    movq	%r12, OSZ_tcb_gpr +  80
    movq	%r13, OSZ_tcb_gpr +  88
    movq	%r14, OSZ_tcb_gpr +  96
    movq	%r15, OSZ_tcb_gpr + 104
    movq	%rbp, OSZ_tcb_gpr + 112
    ret

/* restore thread's context from Thread Control Block */
isr_loadcontext:
    movq	OSZ_tcb_gpr +   0, %rax
    movq	OSZ_tcb_gpr +   8, %rbx
    movq	OSZ_tcb_gpr +  16, %rcx
    movq	OSZ_tcb_gpr +  24, %rdx
    movq	OSZ_tcb_gpr +  32, %rsi
    movq	OSZ_tcb_gpr +  40, %rdi
    movq	OSZ_tcb_gpr +  48, %r8
    movq	OSZ_tcb_gpr +  56, %r9
    movq	OSZ_tcb_gpr +  64, %r10
    movq	OSZ_tcb_gpr +  72, %r11
    movq	OSZ_tcb_gpr +  80, %r12
    movq	OSZ_tcb_gpr +  88, %r13
    movq	OSZ_tcb_gpr +  96, %r14
    movq	OSZ_tcb_gpr + 104, %r15
    movq	OSZ_tcb_gpr + 112, %rbp
    ret

isr_gainentropy:
    /* isr_entropy[isr_ticks[0]&3] ^=
       (isr_entropy[(isr_ticks[0]+1)&3])<<(isr_ticks&7) */
    movq	isr_ticks, %rax
    movq	%rax, %rdx
    incq	%rdx
    movq	%rdx, %rcx
    shlq	$3, %rdx
    andq	$3, %rax
    addq	$isr_entropy, %rax
    addq	$isr_entropy, %rdx
    andb	$0x3f, %cl
    rolq	%cl, (%rax)
    movq	(%rax), %rax
    xorq	%rax, (%rdx)
    ret

/* syscall dispatcher, platform dependent wrapper */
.align	16, 0x90
isr_syscall0:
    cli
    /* tcb->rip */
    movq	%rcx, __PAGESIZE-40
    /* tcb->rflags */
    pushf
    pop     __PAGESIZE-24
    /* enable interrupts flag in rflags */
    orw		$0x200, __PAGESIZE-24
    /* tcb->gpr */
    call	isr_savecontext
    /* 'send' */
    cmpl	$0x646E6573, %eax
    jne		2f
    /* if destionation is SRV_core */
    orq		%rdi, %rdi
    jnz		1f
    call	isr_syscall
    jmp		6f
1:  call	ksend
    jmp		6f
    /* 'recv' */
2:  cmpl	$0x76636572, %eax
    jne		3f
    call	sched_block
    jmp		4f

3:  /* tcb->errno = EINVAL */
    movl	$EINVAL, 672
    /* get a new thread to run */
4:  call	sched_pick
    orq		%rax, %rax
    jz		5f
    movq	%rax, %cr3
5:  call	isr_loadcontext
6:  movq	__PAGESIZE-24, %r11
    movq	__PAGESIZE-40, %rcx
    sysretq


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
    xorl	%eax, %eax
    movl	$0x00130008, %edx
    wrmsr
    /* LSTAR */
    incl	%ecx
    movq	$isr_syscall0, %rax
    movq    %rax, %rdx
    shrq    $32, %rdx
    wrmsr
    ret

/* initialize IRQs, masking all */
isr_initirq:
    /* remap PIC. We have to do this even when PIC is disabled. */
    movb	$0x11, %al
    outb	%al, $PIC_MASTER
    outb	%al, $PIC_SLAVE
    movb	$0x20, %al
    outb	%al, $PIC_MASTER_DATA
    movb	$0x28, %al
    outb	%al, $PIC_SLAVE_DATA
    movb	$0x4, %al
    outb	%al, $PIC_MASTER_DATA
    movb	$0x2, %al
    outb	%al, $PIC_SLAVE_DATA
    movb	$0x1, %al
    outb	%al, $PIC_MASTER_DATA
    outb	%al, $PIC_SLAVE_DATA

    /* PIC init */
    movb	$0xFF, %al
    outb	%al, $PIC_SLAVE_DATA
    btrw	$2, %ax /* enable cascade irq 2 */
    outb	%al, $PIC_MASTER_DATA
    /* enable NMI */
    inb		$0x70, %al
    btrw	$8, %ax
    outb	%al, $0x70
    ret

/* exception handler ISRs */
.align 128, 0x90
isr_exc00divzero:
xchg %bx,%bx
    cli
    callq	isr_savecontext
    xorq	%rdi, %rdi
    movb	$0, %dil
    callq	exc00divzero
    callq	isr_loadcontext
    iretq

.align 128, 0x90
isr_exc01debug:
xchg %bx,%bx
    cli
    callq	isr_savecontext
    xorq	%rdi, %rdi
    movb	$1, %dil
    callq	exc01debug
    callq	isr_loadcontext
    iretq

.align 128, 0x90
isr_exc02nmi:
xchg %bx,%bx
    cli
    callq	isr_savecontext
    xorq	%rdi, %rdi
    movb	$2, %dil
    callq	excabort
    callq	isr_loadcontext
    iretq

.align 128, 0x90
isr_exc03chkpoint:
xchg %bx,%bx
    cli
    callq	isr_savecontext
    xorq	%rdi, %rdi
    movb	$3, %dil
    callq	excabort
    callq	isr_loadcontext
    iretq

.align 128, 0x90
isr_exc04overflow:
xchg %bx,%bx
    cli
    callq	isr_savecontext
    xorq	%rdi, %rdi
    movb	$4, %dil
    callq	excabort
    callq	isr_loadcontext
    iretq

.align 128, 0x90
isr_exc05bound:
xchg %bx,%bx
    cli
    callq	isr_savecontext
    xorq	%rdi, %rdi
    movb	$5, %dil
    callq	excabort
    callq	isr_loadcontext
    iretq

.align 128, 0x90
isr_exc06invopcode:
xchg %bx,%bx
    cli
    callq	isr_savecontext
    xorq	%rdi, %rdi
    movb	$6, %dil
    callq	excabort
    callq	isr_loadcontext
    iretq

.align 128, 0x90
isr_exc07devunavail:
xchg %bx,%bx
    cli
    callq	isr_savecontext
    xorq	%rdi, %rdi
    movb	$7, %dil
    callq	excabort
    callq	isr_loadcontext
    iretq

.align 128, 0x90
isr_exc08dblfault:
xchg %bx,%bx
    cli
    callq	isr_savecontext
    xorq	%rdi, %rdi
    movb	$8, %dil
    callq	excabort
    callq	isr_loadcontext
    iretq

.align 128, 0x90
isr_exc09coproc:
xchg %bx,%bx
    cli
    callq	isr_savecontext
    xorq	%rdi, %rdi
    movb	$9, %dil
    callq	excabort
    callq	isr_loadcontext
    iretq

.align 128, 0x90
isr_exc10invtss:
xchg %bx,%bx
    cli
    callq	isr_savecontext
    xorq	%rdi, %rdi
    movb	$10, %dil
    callq	excabort
    callq	isr_loadcontext
    iretq

.align 128, 0x90
isr_exc11segfault:
xchg %bx,%bx
    cli
    callq	isr_savecontext
    xorq	%rdi, %rdi
    movb	$11, %dil
    callq	excabort
    callq	isr_loadcontext
    iretq

.align 128, 0x90
isr_exc12stackfault:
xchg %bx,%bx
    cli
    callq	isr_savecontext
    xorq	%rdi, %rdi
    movb	$12, %dil
    callq	excabort
    callq	isr_loadcontext
    iretq

.align 128, 0x90
isr_exc13genprot:
xchg %bx,%bx
    cli
    callq	isr_savecontext
    xorq	%rdi, %rdi
    movb	$13, %dil
    callq	exc13genprot
    callq	isr_loadcontext
    iretq

.align 128, 0x90
isr_exc14pagefault:
xchg %bx,%bx
    cli
    callq	isr_savecontext
    xorq	%rdi, %rdi
    movb	$14, %dil
    callq	excabort
    callq	isr_loadcontext
    iretq

.align 128, 0x90
isr_exc15unknown:
xchg %bx,%bx
    cli
    callq	isr_savecontext
    xorq	%rdi, %rdi
    movb	$15, %dil
    callq	excabort
    callq	isr_loadcontext
    iretq

.align 128, 0x90
isr_exc16float:
xchg %bx,%bx
    cli
    callq	isr_savecontext
    xorq	%rdi, %rdi
    movb	$16, %dil
    callq	excabort
    callq	isr_loadcontext
    iretq

.align 128, 0x90
isr_exc17alignment:
xchg %bx,%bx
    cli
    callq	isr_savecontext
    xorq	%rdi, %rdi
    movb	$17, %dil
    callq	excabort
    callq	isr_loadcontext
    iretq

.align 128, 0x90
isr_exc18machinecheck:
xchg %bx,%bx
    cli
    callq	isr_savecontext
    xorq	%rdi, %rdi
    movb	$18, %dil
    callq	excabort
    callq	isr_loadcontext
    iretq

.align 128, 0x90
isr_exc19double:
xchg %bx,%bx
    cli
    callq	isr_savecontext
    xorq	%rdi, %rdi
    movb	$19, %dil
    callq	excabort
    callq	isr_loadcontext
    iretq

.align 128, 0x90
isr_exc20:
xchg %bx,%bx
    cli
    callq	isr_savecontext
    xorq	%rdi, %rdi
    movb	$20, %dil
    callq	excabort
    callq	isr_loadcontext
    iretq

.align 128, 0x90
isr_exc21:
xchg %bx,%bx
    cli
    callq	isr_savecontext
    xorq	%rdi, %rdi
    movb	$21, %dil
    callq	excabort
    callq	isr_loadcontext
    iretq

.align 128, 0x90
isr_exc22:
xchg %bx,%bx
    cli
    callq	isr_savecontext
    xorq	%rdi, %rdi
    movb	$22, %dil
    callq	excabort
    callq	isr_loadcontext
    iretq

.align 128, 0x90
isr_exc23:
xchg %bx,%bx
    cli
    callq	isr_savecontext
    xorq	%rdi, %rdi
    movb	$23, %dil
    callq	excabort
    callq	isr_loadcontext
    iretq

.align 128, 0x90
isr_exc24:
xchg %bx,%bx
    cli
    callq	isr_savecontext
    xorq	%rdi, %rdi
    movb	$24, %dil
    callq	excabort
    callq	isr_loadcontext
    iretq

.align 128, 0x90
isr_exc25:
xchg %bx,%bx
    cli
    callq	isr_savecontext
    xorq	%rdi, %rdi
    movb	$25, %dil
    callq	excabort
    callq	isr_loadcontext
    iretq

.align 128, 0x90
isr_exc26:
xchg %bx,%bx
    cli
    callq	isr_savecontext
    xorq	%rdi, %rdi
    movb	$26, %dil
    callq	excabort
    callq	isr_loadcontext
    iretq

.align 128, 0x90
isr_exc27:
xchg %bx,%bx
    cli
    callq	isr_savecontext
    xorq	%rdi, %rdi
    movb	$27, %dil
    callq	excabort
    callq	isr_loadcontext
    iretq

.align 128, 0x90
isr_exc28:
xchg %bx,%bx
    cli
    callq	isr_savecontext
    xorq	%rdi, %rdi
    movb	$28, %dil
    callq	excabort
    callq	isr_loadcontext
    iretq

.align 128, 0x90
isr_exc29:
xchg %bx,%bx
    cli
    callq	isr_savecontext
    xorq	%rdi, %rdi
    movb	$29, %dil
    callq	excabort
    callq	isr_loadcontext
    iretq

.align 128, 0x90
isr_exc30:
xchg %bx,%bx
    cli
    callq	isr_savecontext
    xorq	%rdi, %rdi
    movb	$30, %dil
    callq	excabort
    callq	isr_loadcontext
    iretq

.align 128, 0x90
isr_exc31:
xchg %bx,%bx
    cli
    callq	isr_savecontext
    xorq	%rdi, %rdi
    movb	$31, %dil
    callq	excabort
    callq	isr_loadcontext
    iretq

/* IRQ handler ISRs */
.align 128, 0x90
isr_irq0:
    /* preemption timer */
iretq
    cli
    call	isr_savecontext
    /* isr_ticks++ */
    addq	$1, isr_ticks
    adcq	$0, isr_ticks+8
    /* tcb->billcnt++ or tcb->syscnt++? */
    movq	56, %rbx
    movb	%al, __PAGESIZE-32   
    cmpb	$0x23, %al
    je		1f
    addq	$8, %rbx
1:  incq	(%rbx)
    /* switch to a new thread if any */
    call	sched_pick
    orq     %rax, %rax
    jz      2f
    movq	%rax, %cr3
    /* PIC EOI */
    movb	$0x20, %al
    outb	%al, $PIC_MASTER_DATA
2:  call	isr_loadcontext
    iretq

.align 128, 0x90
isr_irq1:
    cli
    call	isr_savecontext
    /*call	isr_gainentropy*/
    /* tcb->memroot == sys_mapping? */
    movq	%cr3, %rax
    andw	$0xF000, %ax
xchg %bx, %bx
    cmpq	sys_mapping, %rax
    je		1f
    /* no, switch to system task */
    movq	sys_mapping, %rax
    movq	%rax, %cr3
    /* msg_sends(SRV_core, SYS_IRQ, irq, 0); */
1:  movq	$MQ_ADDRESS, %rdi
    xorq	%rsi, %rsi
    xorq	%rdx, %rdx
    movb	$1, %dl
    xorq	%rcx, %rcx
    callq	ksend
    /*call	isr_irq*/
    /* PIC EOI */
    movb	$0x20, %al
    outb	%al, $PIC_MASTER_DATA
    call	isr_loadcontext
    iretq

.align 128, 0x90
isr_irq2:
    cli
    call	isr_savecontext
    /*call	isr_gainentropy*/
    /* tcb->memroot == sys_mapping? */
    movq	%cr3, %rax
    andw	$0xF000, %ax
xchg %bx, %bx
    cmpq	sys_mapping, %rax
    je		1f
    /* no, switch to system task */
    movq	sys_mapping, %rax
    movq	%rax, %cr3
    /* msg_sends(SRV_core, SYS_IRQ, irq, 0); */
1:  movq	$MQ_ADDRESS, %rdi
    xorq	%rsi, %rsi
    xorq	%rdx, %rdx
    movb	$2, %dl
    xorq	%rcx, %rcx
    callq	ksend
    /*call	isr_irq*/
    /* PIC EOI */
    movb	$0x20, %al
    outb	%al, $PIC_MASTER_DATA
    call	isr_loadcontext
    iretq

.align 128, 0x90
isr_irq3:
    cli
    call	isr_savecontext
    /*call	isr_gainentropy*/
    /* tcb->memroot == sys_mapping? */
    movq	%cr3, %rax
    andw	$0xF000, %ax
xchg %bx, %bx
    cmpq	sys_mapping, %rax
    je		1f
    /* no, switch to system task */
    movq	sys_mapping, %rax
    movq	%rax, %cr3
    /* msg_sends(SRV_core, SYS_IRQ, irq, 0); */
1:  movq	$MQ_ADDRESS, %rdi
    xorq	%rsi, %rsi
    xorq	%rdx, %rdx
    movb	$3, %dl
    xorq	%rcx, %rcx
    callq	ksend
    /*call	isr_irq*/
    /* PIC EOI */
    movb	$0x20, %al
    outb	%al, $PIC_MASTER_DATA
    call	isr_loadcontext
    iretq

.align 128, 0x90
isr_irq4:
    cli
    call	isr_savecontext
    /*call	isr_gainentropy*/
    /* tcb->memroot == sys_mapping? */
    movq	%cr3, %rax
    andw	$0xF000, %ax
xchg %bx, %bx
    cmpq	sys_mapping, %rax
    je		1f
    /* no, switch to system task */
    movq	sys_mapping, %rax
    movq	%rax, %cr3
    /* msg_sends(SRV_core, SYS_IRQ, irq, 0); */
1:  movq	$MQ_ADDRESS, %rdi
    xorq	%rsi, %rsi
    xorq	%rdx, %rdx
    movb	$4, %dl
    xorq	%rcx, %rcx
    callq	ksend
    /*call	isr_irq*/
    /* PIC EOI */
    movb	$0x20, %al
    outb	%al, $PIC_MASTER_DATA
    call	isr_loadcontext
    iretq

.align 128, 0x90
isr_irq5:
    cli
    call	isr_savecontext
    /*call	isr_gainentropy*/
    /* tcb->memroot == sys_mapping? */
    movq	%cr3, %rax
    andw	$0xF000, %ax
xchg %bx, %bx
    cmpq	sys_mapping, %rax
    je		1f
    /* no, switch to system task */
    movq	sys_mapping, %rax
    movq	%rax, %cr3
    /* msg_sends(SRV_core, SYS_IRQ, irq, 0); */
1:  movq	$MQ_ADDRESS, %rdi
    xorq	%rsi, %rsi
    xorq	%rdx, %rdx
    movb	$5, %dl
    xorq	%rcx, %rcx
    callq	ksend
    /*call	isr_irq*/
    /* PIC EOI */
    movb	$0x20, %al
    outb	%al, $PIC_MASTER_DATA
    call	isr_loadcontext
    iretq

.align 128, 0x90
isr_irq6:
    cli
    call	isr_savecontext
    /*call	isr_gainentropy*/
    /* tcb->memroot == sys_mapping? */
    movq	%cr3, %rax
    andw	$0xF000, %ax
xchg %bx, %bx
    cmpq	sys_mapping, %rax
    je		1f
    /* no, switch to system task */
    movq	sys_mapping, %rax
    movq	%rax, %cr3
    /* msg_sends(SRV_core, SYS_IRQ, irq, 0); */
1:  movq	$MQ_ADDRESS, %rdi
    xorq	%rsi, %rsi
    xorq	%rdx, %rdx
    movb	$6, %dl
    xorq	%rcx, %rcx
    callq	ksend
    /*call	isr_irq*/
    /* PIC EOI */
    movb	$0x20, %al
    outb	%al, $PIC_MASTER_DATA
    call	isr_loadcontext
    iretq

.align 128, 0x90
isr_irq7:
    cli
    call	isr_savecontext
    /*call	isr_gainentropy*/
    /* tcb->memroot == sys_mapping? */
    movq	%cr3, %rax
    andw	$0xF000, %ax
xchg %bx, %bx
    cmpq	sys_mapping, %rax
    je		1f
    /* no, switch to system task */
    movq	sys_mapping, %rax
    movq	%rax, %cr3
    /* msg_sends(SRV_core, SYS_IRQ, irq, 0); */
1:  movq	$MQ_ADDRESS, %rdi
    xorq	%rsi, %rsi
    xorq	%rdx, %rdx
    movb	$7, %dl
    xorq	%rcx, %rcx
    callq	ksend
    /*call	isr_irq*/
    /* PIC EOI */
    movb	$0x20, %al
    outb	%al, $PIC_MASTER_DATA
    call	isr_loadcontext
    iretq

.align 128, 0x90
isr_irq8:
    cli
    call	isr_savecontext
    /*call	isr_gainentropy*/
    /* tcb->memroot == sys_mapping? */
    movq	%cr3, %rax
    andw	$0xF000, %ax
xchg %bx, %bx
    cmpq	sys_mapping, %rax
    je		1f
    /* no, switch to system task */
    movq	sys_mapping, %rax
    movq	%rax, %cr3
    /* msg_sends(SRV_core, SYS_IRQ, irq, 0); */
1:  movq	$MQ_ADDRESS, %rdi
    xorq	%rsi, %rsi
    xorq	%rdx, %rdx
    movb	$8, %dl
    xorq	%rcx, %rcx
    callq	ksend
    /*call	isr_irq*/
    /* PIC EOI */
    movb	$0x20, %al
    outb	%al, $PIC_SLAVE_DATA
    outb	%al, $PIC_MASTER_DATA
    call	isr_loadcontext
    iretq

.align 128, 0x90
isr_irq9:
    cli
    call	isr_savecontext
    /*call	isr_gainentropy*/
    /* tcb->memroot == sys_mapping? */
    movq	%cr3, %rax
    andw	$0xF000, %ax
xchg %bx, %bx
    cmpq	sys_mapping, %rax
    je		1f
    /* no, switch to system task */
    movq	sys_mapping, %rax
    movq	%rax, %cr3
    /* msg_sends(SRV_core, SYS_IRQ, irq, 0); */
1:  movq	$MQ_ADDRESS, %rdi
    xorq	%rsi, %rsi
    xorq	%rdx, %rdx
    movb	$9, %dl
    xorq	%rcx, %rcx
    callq	ksend
    /*call	isr_irq*/
    /* PIC EOI */
    movb	$0x20, %al
    outb	%al, $PIC_SLAVE_DATA
    outb	%al, $PIC_MASTER_DATA
    call	isr_loadcontext
    iretq

.align 128, 0x90
isr_irq10:
    cli
    call	isr_savecontext
    /*call	isr_gainentropy*/
    /* tcb->memroot == sys_mapping? */
    movq	%cr3, %rax
    andw	$0xF000, %ax
xchg %bx, %bx
    cmpq	sys_mapping, %rax
    je		1f
    /* no, switch to system task */
    movq	sys_mapping, %rax
    movq	%rax, %cr3
    /* msg_sends(SRV_core, SYS_IRQ, irq, 0); */
1:  movq	$MQ_ADDRESS, %rdi
    xorq	%rsi, %rsi
    xorq	%rdx, %rdx
    movb	$10, %dl
    xorq	%rcx, %rcx
    callq	ksend
    /*call	isr_irq*/
    /* PIC EOI */
    movb	$0x20, %al
    outb	%al, $PIC_SLAVE_DATA
    outb	%al, $PIC_MASTER_DATA
    call	isr_loadcontext
    iretq

.align 128, 0x90
isr_irq11:
    cli
    call	isr_savecontext
    /*call	isr_gainentropy*/
    /* tcb->memroot == sys_mapping? */
    movq	%cr3, %rax
    andw	$0xF000, %ax
xchg %bx, %bx
    cmpq	sys_mapping, %rax
    je		1f
    /* no, switch to system task */
    movq	sys_mapping, %rax
    movq	%rax, %cr3
    /* msg_sends(SRV_core, SYS_IRQ, irq, 0); */
1:  movq	$MQ_ADDRESS, %rdi
    xorq	%rsi, %rsi
    xorq	%rdx, %rdx
    movb	$11, %dl
    xorq	%rcx, %rcx
    callq	ksend
    /*call	isr_irq*/
    /* PIC EOI */
    movb	$0x20, %al
    outb	%al, $PIC_SLAVE_DATA
    outb	%al, $PIC_MASTER_DATA
    call	isr_loadcontext
    iretq

.align 128, 0x90
isr_irq12:
    cli
    call	isr_savecontext
    /*call	isr_gainentropy*/
    /* tcb->memroot == sys_mapping? */
    movq	%cr3, %rax
    andw	$0xF000, %ax
xchg %bx, %bx
    cmpq	sys_mapping, %rax
    je		1f
    /* no, switch to system task */
    movq	sys_mapping, %rax
    movq	%rax, %cr3
    /* msg_sends(SRV_core, SYS_IRQ, irq, 0); */
1:  movq	$MQ_ADDRESS, %rdi
    xorq	%rsi, %rsi
    xorq	%rdx, %rdx
    movb	$12, %dl
    xorq	%rcx, %rcx
    callq	ksend
    /*call	isr_irq*/
    /* PIC EOI */
    movb	$0x20, %al
    outb	%al, $PIC_SLAVE_DATA
    outb	%al, $PIC_MASTER_DATA
    call	isr_loadcontext
    iretq

.align 128, 0x90
isr_irq13:
    cli
    call	isr_savecontext
    /*call	isr_gainentropy*/
    /* tcb->memroot == sys_mapping? */
    movq	%cr3, %rax
    andw	$0xF000, %ax
xchg %bx, %bx
    cmpq	sys_mapping, %rax
    je		1f
    /* no, switch to system task */
    movq	sys_mapping, %rax
    movq	%rax, %cr3
    /* msg_sends(SRV_core, SYS_IRQ, irq, 0); */
1:  movq	$MQ_ADDRESS, %rdi
    xorq	%rsi, %rsi
    xorq	%rdx, %rdx
    movb	$13, %dl
    xorq	%rcx, %rcx
    callq	ksend
    /*call	isr_irq*/
    /* PIC EOI */
    movb	$0x20, %al
    outb	%al, $PIC_SLAVE_DATA
    outb	%al, $PIC_MASTER_DATA
    call	isr_loadcontext
    iretq

.align 128, 0x90
isr_irq14:
    cli
    call	isr_savecontext
    /*call	isr_gainentropy*/
    /* tcb->memroot == sys_mapping? */
    movq	%cr3, %rax
    andw	$0xF000, %ax
xchg %bx, %bx
    cmpq	sys_mapping, %rax
    je		1f
    /* no, switch to system task */
    movq	sys_mapping, %rax
    movq	%rax, %cr3
    /* msg_sends(SRV_core, SYS_IRQ, irq, 0); */
1:  movq	$MQ_ADDRESS, %rdi
    xorq	%rsi, %rsi
    xorq	%rdx, %rdx
    movb	$14, %dl
    xorq	%rcx, %rcx
    callq	ksend
    /*call	isr_irq*/
    /* PIC EOI */
    movb	$0x20, %al
    outb	%al, $PIC_SLAVE_DATA
    outb	%al, $PIC_MASTER_DATA
    call	isr_loadcontext
    iretq

.align 128, 0x90
isr_irq15:
    cli
    call	isr_savecontext
    /*call	isr_gainentropy*/
    /* tcb->memroot == sys_mapping? */
    movq	%cr3, %rax
    andw	$0xF000, %ax
xchg %bx, %bx
    cmpq	sys_mapping, %rax
    je		1f
    /* no, switch to system task */
    movq	sys_mapping, %rax
    movq	%rax, %cr3
    /* msg_sends(SRV_core, SYS_IRQ, irq, 0); */
1:  movq	$MQ_ADDRESS, %rdi
    xorq	%rsi, %rsi
    xorq	%rdx, %rdx
    movb	$15, %dl
    xorq	%rcx, %rcx
    callq	ksend
    /*call	isr_irq*/
    /* PIC EOI */
    movb	$0x20, %al
    outb	%al, $PIC_SLAVE_DATA
    outb	%al, $PIC_MASTER_DATA
    call	isr_loadcontext
    iretq

.align 128, 0x90
isr_irq16:
    cli
    call	isr_savecontext
    /*call	isr_gainentropy*/
    /* tcb->memroot == sys_mapping? */
    movq	%cr3, %rax
    andw	$0xF000, %ax
xchg %bx, %bx
    cmpq	sys_mapping, %rax
    je		1f
    /* no, switch to system task */
    movq	sys_mapping, %rax
    movq	%rax, %cr3
    /* msg_sends(SRV_core, SYS_IRQ, irq, 0); */
1:  movq	$MQ_ADDRESS, %rdi
    xorq	%rsi, %rsi
    xorq	%rdx, %rdx
    movb	$16, %dl
    xorq	%rcx, %rcx
    callq	ksend
    /*call	isr_irq*/
    /* PIC EOI */
    movb	$0x20, %al
    outb	%al, $PIC_SLAVE_DATA
    outb	%al, $PIC_MASTER_DATA
    call	isr_loadcontext
    iretq

