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
.global isr_initirq
.global isr_enableirq
.global isr_disableirq

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
apic:
    .quad	0
ioapic:
    .quad	0
ctrl:
    .asciz "APIC"
    .align 8
nopic:
    .asciz	"%s not supported"
ioctrl:
    .asciz	"IOAPIC"

.section .text
/* uint64_t ioapic_read(uint32_t port) */
ioapic_read:
    incl	%edi
    movl	%edi, ioapic
    movl	ioapic+16, %eax
    shlq	$32, %rax
    decl	%edi
    movl	%edi, ioapic
    movl	ioapic+16, %eax
    ret

/* void ioapic_write(uint32_t port, uint64_t value) */
ioapic_write:
    movq	%rsi, %rax
    shrq	$32, %rax
    incl	%edi
    movl	%edi, ioapic
    movl	%eax, ioapic+16
    decl	%edi
    movl	%edi, ioapic
    movl	%esi, ioapic+16
    ret

/* void isr_enableirq(uint64_t irq) */
isr_enableirq:
    movq	%rdi, %rax
    addq	$32, %rax
    orq		$IOAPIC_IRQMASK, %rax
    btrl	$16, %eax
    movq	%rdi, %rdx
    shlq	$1, %rdx
    addq	$IOAPIC_IRQ0, %rdx
    /* write the high dword first */
    movq	%rax, %rcx
    shrq	$32, %rcx
    incl	%edx
    movq	ioapic, %rsi
    movl	%edx, (%rsi)
    movl	%ecx, 16(%rsi)
    /* and then the low with unmask bit */
    decl	%edx
    movl	%edx, (%rsi)
    movl	%eax, 16(%rsi)
    ret

/* void isr_disableirq(uint64_t irq) */
isr_disableirq:
    movq	%rdi, %rax
    addq	$32, %rax
    orq		$IOAPIC_IRQMASK, %rax
    btsl	$16, %eax
    movq	%rdi, %rdx
    shlq	$1, %rdx
    addq	$IOAPIC_IRQ0, %rdx
    /* write the low dword first with mask bit */
    movq	ioapic, %rsi
    movl	%edx, (%rsi)
    movl	%eax, 16(%rsi)
    shrq	$32, %rax
    /* and then the high dword */
    incl	%edx
    movl	%edx, (%rsi)
    movl	%eax, 16(%rsi)
    ret

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
    ret

isr_initirq:
/* initialize IRQs, masking all */
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
    movb	$0xFF, %al
    outb	%al, $PIC_MASTER_DATA
    outb	%al, $PIC_SLAVE_DATA
    /* IOAPIC init */
    movq	ioapic_addr, %rax
    orq 	%rax, %rax
    jnz  	1f
    movq	$nopic, %rdi
    movq	$ioctrl, %rsi
    call	kpanic
    1:
    /* map ioapic at pmm.bss_end and increase bss pointer */
    movq	pmm + 40, %rdi
    movq	%rdi, ioapic
    movq	%rax, %rsi
    movb	$PG_CORE_NOCACHE, %dl
    call	kmap
    addq	$4096, pmm + 40

    /* setup IRQs, mask them all */
    xorq	%rdi, %rdi
1:  call	isr_disableirq
    incl	%edi
    cmpl	$16, %edi
    jb		1b

    /* enable IOAPIC in IMCR */
    movb	$0x70, %al
    outb	%al, $0x22
    movb	$1, %al
    outb	%al, $0x23
    /* APIC init */
    xorq	%rax, %rax
    incb	%al
    cpuid
    btl 	$9, %edx
    jc  	1f
    movq	$nopic, %rdi
    movq	$ctrl, %rsi
    call	kpanic
    1:
    /* find apic physical address */
    movq	lapic_addr, %rax
    orq		%rax, %rax
    jnz		1f
    movl	$0x1B, %ecx
    rdmsr
    andw	$0xF000, %ax
    1:
    /* map apic at pmm.bss_end and increase bss pointer */
    movq	pmm + 40, %rdi
    movq	%rdi, apic
    movq	%rax, %rsi
    movb	$PG_CORE_NOCACHE, %dl
    call	kmap
    addq	$4096, pmm + 40
    /* setup */
    movl	$0xFFFFFFFF, apic + APIC_DFR
    movl	apic + APIC_DFR, %eax
    andl	$0x00FFFFFF, %eax
    orb		$1, %al
    movl	%eax, apic + APIC_LDR
    movl	$APIC_DISABLE, apic + APIC_LVT_TMR
    movl	$APIC_DISABLE, apic + APIC_LVT_LINT0
    movl	$APIC_DISABLE, apic + APIC_LVT_LINT1
    movl	$IOAPIC_NMI, apic + APIC_LVT_PERF
    movl	$0, apic + APIC_TASKPRI
    /* enable */
    movl	$0x1B, %ecx
    rdmsr
    btsl	$11, %eax
    wrmsr
    /* sw enable 
    movl	$APIC_SW_ENABLE+39, apic + APIC_SPURIOUS */
    /* enable NMI */
    inb		$0x70, %al
    btrw	$8, %ax
    outb	%al, $0x70
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
    xchg %bx,%bx
    cli
    lock
    btsq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    call	isr_savecontext
    xorq	%rdi, %rdi
    call	isr_irq
    /* APIC EOI */
    movq	apic, %rax
    movl	$0, 0xB0(%rax)
    call	isr_loadcontext
    lock
    btrq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    iretq

.align 128, 0x90
isr_irq1:
    xchg %bx,%bx
    cli
    lock
    btsq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    pushq	%rdi
    pushq	%rax
    call	isr_savecontext
    xorq	%rdi, %rdi
    movb	$1, %dil
    /*call	isr_irq*/
    /* APIC EOI */
    movq	apic, %rax
    movl	$0, 0xB0(%rax)
    call	isr_loadcontext
    popq	%rax
    popq	%rdi
    lock
    btrq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    iretq

.align 128, 0x90
isr_irq2:
    xchg %bx,%bx
    cli
    lock
    btsq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    pushq	%rdi
    pushq	%rax
    call	isr_savecontext
    xorq	%rdi, %rdi
    movb	$2, %dil
    /*call	isr_irq*/
    /* APIC EOI */
    movq	apic, %rax
    movl	$0, 0xB0(%rax)
    call	isr_loadcontext
    popq	%rax
    popq	%rdi
    lock
    btrq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    iretq

.align 128, 0x90
isr_irq3:
    xchg %bx,%bx
    cli
    lock
    btsq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    pushq	%rdi
    pushq	%rax
    call	isr_savecontext
    xorq	%rdi, %rdi
    movb	$3, %dil
    /*call	isr_irq*/
    /* APIC EOI */
    movq	apic, %rax
    movl	$0, 0xB0(%rax)
    call	isr_loadcontext
    popq	%rax
    popq	%rdi
    lock
    btrq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    iretq

.align 128, 0x90
isr_irq4:
    xchg %bx,%bx
    cli
    lock
    btsq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    pushq	%rdi
    pushq	%rax
    call	isr_savecontext
    xorq	%rdi, %rdi
    movb	$4, %dil
    /*call	isr_irq*/
    /* APIC EOI */
    movq	apic, %rax
    movl	$0, 0xB0(%rax)
    call	isr_loadcontext
    popq	%rax
    popq	%rdi
    lock
    btrq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    iretq

.align 128, 0x90
isr_irq5:
    xchg %bx,%bx
    cli
    lock
    btsq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    pushq	%rdi
    pushq	%rax
    call	isr_savecontext
    xorq	%rdi, %rdi
    movb	$5, %dil
    /*call	isr_irq*/
    /* APIC EOI */
    movq	apic, %rax
    movl	$0, 0xB0(%rax)
    call	isr_loadcontext
    popq	%rax
    popq	%rdi
    lock
    btrq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    iretq

.align 128, 0x90
isr_irq6:
    xchg %bx,%bx
    cli
    lock
    btsq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    pushq	%rdi
    pushq	%rax
    call	isr_savecontext
    xorq	%rdi, %rdi
    movb	$6, %dil
    /*call	isr_irq*/
    /* APIC EOI */
    movq	apic, %rax
    movl	$0, 0xB0(%rax)
    call	isr_loadcontext
    popq	%rax
    popq	%rdi
    lock
    btrq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    iretq

.align 128, 0x90
isr_irq7:
    xchg %bx,%bx
    cli
    lock
    btsq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    pushq	%rdi
    pushq	%rax
    call	isr_savecontext
    xorq	%rdi, %rdi
    movb	$7, %dil
    /*call	isr_irq*/
    /* APIC EOI */
    movq	apic, %rax
    movl	$0, 0xB0(%rax)
    call	isr_loadcontext
    popq	%rax
    popq	%rdi
    lock
    btrq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    iretq

.align 128, 0x90
isr_irq8:
    xchg %bx,%bx
    cli
    lock
    btsq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    pushq	%rdi
    pushq	%rax
    call	isr_savecontext
    xorq	%rdi, %rdi
    movb	$8, %dil
    /*call	isr_irq*/
    /* APIC EOI */
    movq	apic, %rax
    movl	$0, 0xB0(%rax)
    call	isr_loadcontext
    popq	%rax
    popq	%rdi
    lock
    btrq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    iretq

.align 128, 0x90
isr_irq9:
    xchg %bx,%bx
    cli
    lock
    btsq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    pushq	%rdi
    pushq	%rax
    call	isr_savecontext
    xorq	%rdi, %rdi
    movb	$9, %dil
    /*call	isr_irq*/
    /* APIC EOI */
    movq	apic, %rax
    movl	$0, 0xB0(%rax)
    call	isr_loadcontext
    popq	%rax
    popq	%rdi
    lock
    btrq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    iretq

.align 128, 0x90
isr_irq10:
    xchg %bx,%bx
    cli
    lock
    btsq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    pushq	%rdi
    pushq	%rax
    call	isr_savecontext
    xorq	%rdi, %rdi
    movb	$10, %dil
    /*call	isr_irq*/
    /* APIC EOI */
    movq	apic, %rax
    movl	$0, 0xB0(%rax)
    call	isr_loadcontext
    popq	%rax
    popq	%rdi
    lock
    btrq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    iretq

.align 128, 0x90
isr_irq11:
    xchg %bx,%bx
    cli
    lock
    btsq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    pushq	%rdi
    pushq	%rax
    call	isr_savecontext
    xorq	%rdi, %rdi
    movb	$11, %dil
    /*call	isr_irq*/
    /* APIC EOI */
    movq	apic, %rax
    movl	$0, 0xB0(%rax)
    call	isr_loadcontext
    popq	%rax
    popq	%rdi
    lock
    btrq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    iretq

.align 128, 0x90
isr_irq12:
    xchg %bx,%bx
    cli
    lock
    btsq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    pushq	%rdi
    pushq	%rax
    call	isr_savecontext
    xorq	%rdi, %rdi
    movb	$12, %dil
    /*call	isr_irq*/
    /* APIC EOI */
    movq	apic, %rax
    movl	$0, 0xB0(%rax)
    call	isr_loadcontext
    popq	%rax
    popq	%rdi
    lock
    btrq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    iretq

.align 128, 0x90
isr_irq13:
    xchg %bx,%bx
    cli
    lock
    btsq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    pushq	%rdi
    pushq	%rax
    call	isr_savecontext
    xorq	%rdi, %rdi
    movb	$13, %dil
    /*call	isr_irq*/
    /* APIC EOI */
    movq	apic, %rax
    movl	$0, 0xB0(%rax)
    call	isr_loadcontext
    popq	%rax
    popq	%rdi
    lock
    btrq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    iretq

.align 128, 0x90
isr_irq14:
    xchg %bx,%bx
    cli
    lock
    btsq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    pushq	%rdi
    pushq	%rax
    call	isr_savecontext
    xorq	%rdi, %rdi
    movb	$14, %dil
    /*call	isr_irq*/
    /* APIC EOI */
    movq	apic, %rax
    movl	$0, 0xB0(%rax)
    call	isr_loadcontext
    popq	%rax
    popq	%rdi
    lock
    btrq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    iretq

.align 128, 0x90
isr_irq15:
    xchg %bx,%bx
    cli
    lock
    btsq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    pushq	%rdi
    pushq	%rax
    call	isr_savecontext
    xorq	%rdi, %rdi
    movb	$15, %dil
    /*call	isr_irq*/
    /* APIC EOI */
    movq	apic, %rax
    movl	$0, 0xB0(%rax)
    call	isr_loadcontext
    popq	%rax
    popq	%rdi
    lock
    btrq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    iretq

.align 128, 0x90
isr_irq16:
    xchg %bx,%bx
    cli
    lock
    btsq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    pushq	%rdi
    pushq	%rax
    call	isr_savecontext
    xorq	%rdi, %rdi
    movb	$16, %dil
    /*call	isr_irq*/
    /* APIC EOI */
    movq	apic, %rax
    movl	$0, 0xB0(%rax)
    call	isr_loadcontext
    popq	%rax
    popq	%rdi
    lock
    btrq	$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
    iretq

