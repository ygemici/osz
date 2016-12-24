#!/bin/sh

#
# core/x86_64/isrs.sh
#
# Copyright 2016 CC-by-nc-sa-4.0 bztsrc@github
# https://creativecommons.org/licenses/by-nc-sa/4.0/
# 
# You are free to:
#
# - Share — copy and redistribute the material in any medium or format
# - Adapt — remix, transform, and build upon the material
#     The licensor cannot revoke these freedoms as long as you follow
#     the license terms.
# 
# Under the following terms:
#
# - Attribution — You must give appropriate credit, provide a link to
#     the license, and indicate if changes were made. You may do so in
#     any reasonable manner, but not in any way that suggests the
#     licensor endorses you or your use.
# - NonCommercial — You may not use the material for commercial purposes.
# - ShareAlike — If you remix, transform, or build upon the material,
#     you must distribute your contributions under the same license as
#     the original.
#
# @brief Shell script to generate isrs.S
#

numirq=16
read -r -d '' exceptions <<EOF
exc00divzero
exc01debug
exc02nmi
exc03chkpoint
exc04overflow
exc05bound
exc06invopcode
exc07devunavail
exc08dblfault
exc09coproc
exc10invtss
exc11segfault
exc12stackfault
exc13genprot
exc14pagefault
exc15unknown
exc16float
exc17alignment
exc18machinecheck
exc19double
exc20
exc21
exc22
exc23
exc24
exc25
exc26
exc27
exc28
exc29
exc30
exc31
EOF

numirq=`grep "ISR_NUMIRQ" isr.h|cut -d ' ' -f 3`
numhnd=`grep "ISR_NUMHANDLER" isr.h|cut -d ' ' -f 3`
isrmax=`grep "ISR_MAX" isr.h|cut -d ' ' -f 3`
ctrl=`grep "ISR_CTRL" isr.h|cut -d ' ' -f 3`
idtsz=$[(($numirq*$numhnd*8+4095)/4096)*4096]
echo "  gen		src/core/$1/isrs.S (${ctrl:5}, numirq $numirq (x$numhnd), idtsz $idtsz)"
cat >isrs.S <<EOF
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
    .word	(32+$numirq)*16-1
    .quad	0
    .align	8
isr_preemptcnt:
    .quad	0
EOF
if [ "$ctrl" == "CTRL_PIC" ]; then
	cat >>isrs.S <<-EOF
	ctrl:
	    .asciz "PIC"
	EOF
else
	if [ "$ctrl" == "CTRL_x2APIC" ]; then
		cat >>isrs.S <<-EOF
		ioapic:
		    .quad	0
		ctrl:
		    .asciz "x2APIC"
		EOF
	else
		cat >>isrs.S <<-EOF
		apic:
		    .quad	0
		ioapic:
		    .quad	0
		ctrl:
		    .asciz "APIC"
		EOF
	fi
fi
cat >>isrs.S <<EOF
    .align 8
nopic:
    .asciz	"%s not supported"
ioctrl:
    .asciz	"IOAPIC"

.section .text
EOF
if [ "$ctrl" == "CTRL_PIC" ]; then
	cat >>isrs.S <<-EOF
	/* void isr_enableirq(uint64_t irq) */
	isr_enableirq:
	    cmpl	\$2, %edi
	    je		1f
	    xorl	%edx, %edx
	    incl	%edx
	    movl	%edi, %ecx
	    cmpl	\$8, %ecx
	    jae		2f
	    shll	%cl, %edx
	    notl	%edx
	    inb		\$PIC_MASTER_DATA, %al
	    andb	%dl, %al
	    outb	%al, \$PIC_MASTER_DATA
	1:  ret
	2:  subl	\$8, %edi
	    shll	%cl, %edx
	    notl	%edx
	    inb		\$PIC_SLAVE_DATA, %al
	    andb	%cl, %al
	    outb	%al, \$PIC_SLAVE_DATA
	    ret
	
	/* void isr_disableirq(uint64_t irq) */
	isr_disableirq:
	    cmpl	\$2, %edi
	    je		1f
	    xorl	%edx, %edx
	    incl	%edx
	    movl	%edi, %ecx
	    cmpl	\$8, %edi
	    jae		2f
	    shll	%cl, %edx
	    inb		\$PIC_MASTER_DATA, %al
	    orb		%cl, %al
	    outb	%al, \$PIC_MASTER_DATA
	1:  ret
	2:  subl	\$8, %edi
	    shll	%cl, %edx
	    inb		\$PIC_SLAVE_DATA, %al
	    orb		%cl, %al
	    outb	%al, \$PIC_SLAVE_DATA
	    ret
	
	EOF
else
	cat >>isrs.S <<-EOF
	/* uint64_t ioapic_read(uint32_t port) */
	ioapic_read:
	    incl	%edi
	    movl	%edi, ioapic
	    movl	ioapic+16, %eax
	    shlq	\$32, %rax
	    decl	%edi
	    movl	%edi, ioapic
	    movl	ioapic+16, %eax
	    ret
	
	/* void ioapic_write(uint32_t port, uint64_t value) */
	ioapic_write:
	    movq	%rsi, %rax
	    shrq	\$32, %rax
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
	    addq	\$32, %rax
	    orq		\$IOAPIC_IRQMASK, %rax
	    btrl	\$16, %eax
	    movq	%rdi, %rdx
	    shlq	\$1, %rdx
	    addq	\$IOAPIC_IRQ0, %rdx
	    /* write the high dword first */
	    movq	%rax, %rcx
	    shrq	\$32, %rcx
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
	    addq	\$32, %rax
	    orq		\$IOAPIC_IRQMASK, %rax
	    btsl	\$16, %eax
	    movq	%rdi, %rdx
	    shlq	\$1, %rdx
	    addq	\$IOAPIC_IRQ0, %rdx
	    /* write the low dword first with mask bit */
	    movq	ioapic, %rsi
	    movl	%edx, (%rsi)
	    movl	%eax, 16(%rsi)
	    shrq	\$32, %rax
	    /* and then the high dword */
	    incl	%edx
	    movl	%edx, (%rsi)
	    movl	%eax, 16(%rsi)
	    ret
	
	EOF
fi
cat >>isrs.S <<EOF
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
    shlq	\$3, %rdx
    andq	\$3, %rax
    addq	\$isr_entropy, %rax
    addq	\$isr_entropy, %rdx
    andb	\$0x3f, %cl
    rolq	%cl, (%rax)
    movq	(%rax), %rax
    xorq	%rax, (%rdx)
    ret

/* syscall dispatcher */
.align	16, 0x90
isr_syscall0:
    cli
xchg %bx,%bx
    /* tcb->rip */
    movq	%rcx, __PAGESIZE-40
    /* tcb->rflags */
    pushf
    pop     __PAGESIZE-24
    /* tcb->gpr */
    call	isr_savecontext
    /* 'send' */
    cmpl    \$0x646E6573, %eax
    jne     2f
    /* if destionation is SRV_core */
    orq     %rdi, %rdi
    jnz     1f
    call	isr_syscall
    jmp     5f
1:  call    ksend
    jmp     5f
    /* 'recv' */
2:  cmpl    \$0x76636572, %eax
    jne     3f
    call    sched_block
    jmp     4f

3:  /* tcb->errno = EINVAL */
    movl    \$EINVAL, 672
    /* get a new thread to run */
4:  call	sched_pick
    movq	%rax, %cr3
    call	isr_loadcontext
5:  movq    __PAGESIZE-24, %r11
    movq    __PAGESIZE-40, %rcx
    sysretq


isr_initirq:
/* TSS64 descriptor in GDT */
    movq	\$gdt64_tss, %rbx
    movl	%esi, %eax
    andl	\$0xFFFFFF, %eax
    addl	%eax, 2(%rbx)
    movq	%rsi, %rax
    shrq	\$24, %rax
    addq	%rax, 7(%rbx)
/* setup task register */
    movl	\$0x28 + 3, %eax
    ltr		%ax
/* IDTR */
    movq	\$idt64, %rax
    movq	%rdi, 2(%rax)
    lidt	(%rax)
/* setup syscall dispatcher */
    /* STAR */
    xorq	%rcx, %rcx
    movl	\$0xC0000081, %ecx
    movq	\$0x0023000800000000, %rax
    wrmsr
    /* LSTAR */
    incl	%ecx
    movq	\$isr_syscall0, %rax
    movq    %rax, %rdx
    shrq    \$32, %rdx
    wrmsr
    ret
/* initialize IRQs, masking all */
    /* remap PIC. We have to do this even when PIC is disabled. */
    movb	\$0x11, %al
    outb	%al, \$PIC_MASTER
    outb	%al, \$PIC_SLAVE
    movb	\$0x20, %al
    outb	%al, \$PIC_MASTER_DATA
    movb	\$0x28, %al
    outb	%al, \$PIC_SLAVE_DATA
    movb	\$0x4, %al
    outb	%al, \$PIC_MASTER_DATA
    movb	\$0x2, %al
    outb	%al, \$PIC_SLAVE_DATA
    movb	\$0x1, %al
    outb	%al, \$PIC_MASTER_DATA
    outb	%al, \$PIC_SLAVE_DATA
EOF
if [ "$ctrl" == "CTRL_PIC" ]; then
	cat >>isrs.S <<-EOF
	    /* PIC init */
	    movb	\$0xFF, %al
	    outb	%al, \$PIC_SLAVE_DATA
	    btrw	\$2, %ax /* enable cascade irq 2 */
	    outb	%al, \$PIC_MASTER_DATA
	EOF
	read -r -d '' EOI <<-EOF
	    /* PIC EOI */
	    movb	\$0x20, %al
	    outb	%al, \$PIC_MASTER_DATA
	EOF
	read -r -d '' EOI2 <<-EOF
	    /* PIC EOI */
	    movb	\$0x20, %al
	    outb	%al, \$PIC_SLAVE_DATA
	    outb	%al, \$PIC_MASTER_DATA
	EOF
else
	cat >>isrs.S <<-EOF
	    movb	\$0xFF, %al
	    outb	%al, \$PIC_MASTER_DATA
	    outb	%al, \$PIC_SLAVE_DATA
	    /* IOAPIC init */
	    movq	ioapic_addr, %rax
	    orq 	%rax, %rax
	    jnz  	1f
	    movq	\$nopic, %rdi
	    movq	\$ioctrl, %rsi
	    call	kpanic
	    1:
	    /* map ioapic at pmm.bss_end and increase bss pointer */
	    movq	pmm + 40, %rdi
	    movq	%rdi, ioapic
	    movq	%rax, %rsi
	    movb	\$PG_CORE_NOCACHE, %dl
	    call	kmap
	    addq	\$4096, pmm + 40
	
	    /* setup IRQs, mask them all */
	    xorq	%rdi, %rdi
	1:  call	isr_disableirq
	    incl	%edi
	    cmpl	\$$numirq, %edi
	    jb		1b
	
	    /* enable IOAPIC in IMCR */
	    movb	\$0x70, %al
	    outb	%al, \$0x22
	    movb	\$1, %al
	    outb	%al, \$0x23
	EOF
	if [ "$ctrl" == "CTRL_x2APIC" ]; then
		cat >>isrs.S <<-EOF
		    /* x2APIC init */
		    xorq	%rax, %rax
		    incb	%al
		    cpuid
		    btl 	\$21, %edx
		    jc  	1f
		    movq	\$nopic, %rdi
		    movq	\$ctrl, %rsi
		    call	kpanic
		    1:
		    movl	\$0x1B, %ecx
		    rdmsr
		    btsl	\$10, %eax
		    btsl	\$11, %eax
		    wrmsr
		EOF
		read -r -d '' EOI <<-EOF
		    /* x2APIC EOI */
		    xorl	%eax, %eax
		    movl	\$0x80B, %ecx
		    wrmsr
		EOF
	else
		cat >>isrs.S <<-EOF
		    /* APIC init */
		    xorq	%rax, %rax
		    incb	%al
		    cpuid
		    btl 	\$9, %edx
		    jc  	1f
		    movq	\$nopic, %rdi
		    movq	\$ctrl, %rsi
		    call	kpanic
		    1:
		    /* find apic physical address */
		    movq	lapic_addr, %rax
		    orq		%rax, %rax
		    jnz		1f
		    movl	\$0x1B, %ecx
		    rdmsr
		    andw	\$0xF000, %ax
		    1:
		    /* map apic at pmm.bss_end and increase bss pointer */
		    movq	pmm + 40, %rdi
		    movq	%rdi, apic
		    movq	%rax, %rsi
		    movb	\$PG_CORE_NOCACHE, %dl
		    call	kmap
		    addq	\$4096, pmm + 40
		    /* setup */
		    movl	\$0xFFFFFFFF, apic + APIC_DFR
		    movl	apic + APIC_DFR, %eax
		    andl	\$0x00FFFFFF, %eax
		    orb		\$1, %al
		    movl	%eax, apic + APIC_LDR
		    movl	\$APIC_DISABLE, apic + APIC_LVT_TMR
		    movl	\$APIC_DISABLE, apic + APIC_LVT_LINT0
		    movl	\$APIC_DISABLE, apic + APIC_LVT_LINT1
		    movl	\$IOAPIC_NMI, apic + APIC_LVT_PERF
		    movl	\$0, apic + APIC_TASKPRI
		    /* enable */
		    movl	\$0x1B, %ecx
		    rdmsr
		    btsl	\$11, %eax
		    wrmsr
		    /* sw enable 
		    movl	\$APIC_SW_ENABLE+39, apic + APIC_SPURIOUS */
		EOF
		read -r -d '' EOI <<-EOF
		    /* APIC EOI */
		    movq	apic, %rax
		    movl	\$0, 0xB0(%rax)
		EOF
	fi
fi

cat >>isrs.S <<EOF
    /* enable NMI */
    inb		\$0x70, %al
    btrw	\$8, %ax
    outb	%al, \$0x70
    ret

/* exception handler ISRs */
EOF
i=0
for isr in $exceptions
do
	echo "			$isr" >&2
	handler=`grep " $isr" isr.c|cut -d '(' -f 1|cut -d ' ' -f 2`
	if [ "$handler" == "" ]; then
		handler="excabort"
	fi
	cat >>isrs.S <<-EOF
	.align $isrmax, 0x90
	isr_$isr:
xchg %bx,%bx
	    cli
	    callq	isr_savecontext
	    xorq	%rdi, %rdi
	    movb	\$$i, %dil
	    callq	$handler
	    callq	isr_loadcontext
	    iretq
	
	EOF
	export i=$[$i+1];
done
echo "			irq0" >&2
cat >>isrs.S <<EOF
/* IRQ handler ISRs */
.align $isrmax, 0x90
isr_irq0:
    /* preemption timer */
    cli
    call	isr_savecontext
    /* isr_ticks++ */
    addq	\$1, isr_ticks
    adcq	\$0, isr_ticks+8
    /* tcb->billcnt++ or tcb->syscnt++? */
    movq	$656, %rbx
    movb	%al, __PAGESIZE-32   
    cmpb	\$0x23, %al
    je		1f
    addq	\$8, %rbx
1:  incq	(%rbx)
    /* switch to a new thread */
    call	sched_pick
    movq	%rax, %cr3
    $EOI
    call	isr_loadcontext
    iretq

EOF
for isr in `seq 1 $numirq`
do
	echo "			irq$isr" >&2
	if [ "$EOI2" != "" -a $isr -gt 7 ]
	then
		EOI="$EOI2";
	fi
	cat >>isrs.S <<-EOF
	.align $isrmax, 0x90
	isr_irq$isr:
	    cli
	    call	isr_savecontext
	    /*call	isr_gainentropy*/
	    /* tcb->memroot == sys_mapping? */
	    movq	%cr3, %rax
	    andw	\$0xF000, %ax
	    cmpq	sys_mapping, %rax
	    je		1f
	    /* no, switch to system task */
	    movq	sys_mapping, %rax
	    movq	%rax, %cr3
	    /* msg_sends(SRV_core, SYS_IRQ, irq, 0); */
	1:  movq	\$MQ_ADDRESS, %rdi
	    xorq	%rsi, %rsi
	    xorq	%rdx, %rdx
	    movb	\$$isr, %dl
	    xorq	%rcx, %rcx
	    callq	ksend
	    /*call	isr_irq*/
	    $EOI
	    call	isr_loadcontext
	    iretq
	
	EOF
done
