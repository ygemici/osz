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
EOF

isrmax=`grep "ISR_MAX" isr.h|cut -d ' ' -f 3`
isrstack=`grep "ISR_STACK" isr.h|cut -d ' ' -f 3`
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
    .word	(32+$numirq)*16-1
    .quad	0
    .align	8
EOF
if [ "$1" == "pic" ]; then
	cat >>isrs.S <<-EOF
	timer:
	    .asciz "PIC,PIT"
	EOF
else
	if [ "$1" == "x2apic" ]; then
		cat >>isrs.S <<-EOF
		timer:
		    .asciz "x2APIC"
		EOF
	else
		cat >>isrs.S <<-EOF
		apic:
		    .quad	0
		timer:
		    .asciz "APIC"
		EOF
	fi
fi
cat >>isrs.S <<EOF
    .align 8
nopic:
    .asciz	"%s not supported"
.section .text

/* store thread's context into Thread Control Block */
isr_savecontext:
    ret

/* restore thread's context from Thread Control Block */
isr_loadcontext:
    ret

isr_initgates:
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
    movq	\$0x0013000800000000, %rax
    wrmsr
    /* LSTAR */
    inc		%ecx
    xorl	%edx, %edx
    movq	\$isr_syscall, %rax
    wrmsr
/* enable IRQs */
EOF
if [ "$1" == "pic" ]; then
	cat >>isrs.S <<-EOF
	    /* PIC init */
	    movb	\$0x11, %al
	    outb	%al, \$0x20
	    outb	%al, \$0xA0
	    movb	\$0x20, %al
	    outb	%al, \$0x21
	    movb	\$0x28, %al
	    outb	%al, \$0xA1
	    movb	\$0x4, %al
	    outb	%al, \$0x21
	    movb	\$0x2, %al
	    outb	%al, \$0xA1
	    movb	\$0x1, %al
	    outb	%al, \$0x21
	    outb	%al, \$0xA1
	    movb	\$0xFF, %al
	    outb	%al, \$0x21
	    outb	%al, \$0xA1
	    /* RTC init */
	EOF
	read -r -d '' EOI <<-EOF
	    /* PIC EOI */
	    movb	\$0x20, %al
	    outb	%al, \$0x20
	EOF
else
	if [ "$1" == "x2apic" ]; then
		cat >>isrs.S <<-EOF
		    /* x2APIC init */
		    xorq	%rax, %rax
		    incb	%al
		    cpuid
		    btl 	\$21, %edx
		    jc  	1f
		    movq	\$nopic, %rdi
		    movq	\$timer, %rsi
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
		    movq	\$timer, %rsi
		    call	kpanic
		    1:
		    /* find apic physical address */
		    movl	\$0x1B, %ecx
		    rdmsr
		    andw	\$0xF000, %ax
		    /* map apic at pmm.bss_end and increase bss pointer */
		    movq	pmm + 40, %rdi
		    movq	%rdi, apic
		    movq	%rax, %rsi
		    movb	\$PG_CORE_NOCACHE, %dl
		    call	kmap
		    addq	\$4096, pmm + 40
		    /* enable */
		    movl	\$0x1B, %ecx
		    rdmsr
		    btsl	\$11, %eax
		    wrmsr
		EOF
		read -r -d '' EOI <<-EOF
		    /* APIC EOI */
		    movq	apic, %rax
		    movl	\$0, 0xB0(%rax)
		EOF
	fi
	cat >>isrs.S <<-EOF
	    /* IOAPIC init */
	    
	EOF
fi

cat >>isrs.S <<EOF
    ret

/* syscall dispatcher */
.align	16
isr_syscall:
    sysret

/* exception handler ISRs */
EOF
i=0
for isr in $exceptions
do
	echo $isr
	handler=`grep " $isr" isr.c|cut -d '(' -f 1|cut -d ' ' -f 2`
	if [ "$handler" == "" ]; then
		handler="excabort"
	fi
	cat >>isrs.S <<-EOF
	.align $isrmax
	isr_$isr:
	    cli
	    lock
	    btsq	\$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
	    callq	isr_savecontext
	    xorq	%rdi, %rdi
	    movb	\$$i, %dil
	    callq	$handler
	    callq	isr_loadcontext
	    lock
	    btrq	\$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
	    sti
	    iretq
	
	EOF
	export i=$[$i+1];
done
cat >>isrs.S <<EOF
/* IRQ handler ISRs */
EOF
for isr in `seq 0 $numirq`
do
	echo irq$isr
	cat >>isrs.S <<-EOF
	.align $isrmax
	isr_irq$isr:
	    cli
	    lock
	    btsq	\$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
	    call	isr_savecontext
	    xorq	%rdi, %rdi
	    movb	\$$isr, %dil
	    call	isr_irq
	    $EOI
	    call	isr_loadcontext
	    lock
	    btrq	\$LOCK_TASKSWITCH, ccb + MUTEX_OFFS
	    sti
	    iretq
	
	EOF
done
