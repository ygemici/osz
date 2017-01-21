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

# this file uses tabs intentionally. Required by cat <<-EOF

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
isrexcmax=`grep "ISR_EXCMAX" isr.h|cut -d ' ' -f 3`
isrirqmax=`grep "ISR_IRQMAX" isr.h|cut -d ' ' -f 3`
isrstack=`grep "ISR_STACK" isr.h|cut -d ' ' -f 3`
isrtimer=`grep "ISR_IRQTMR" isr.h|cut -d ' ' -f 3`
ctrl=`grep "ISR_CTRL" isr.h|head -1|cut -d ' ' -f 3`
idtsz=$[(($numirq*$numhnd*8+4095)/4096)*4096]
DEBUG=`grep "DEBUG" ../../../Config |cut -d ' ' -f 3`
OPTIMIZE=`grep "OPTIMIZE" ../../../Config |cut -d ' ' -f 3`
if [ "$DEBUG" -eq "1" ]; then
    DBG="xchg %bx, %bx"
else
    DBG=""
fi
if [ "$OPTIMIZE" -eq "1" -a "$ctrl" == "CTRL_APIC" ]; then
    ctrl="CTRL_x2APIC"
fi
echo "  gen		src/core/$1/isrs.S (${ctrl:5}, numirq $numirq (x$numhnd), idtsz $idtsz)"

cat >isrs.S <<EOF
/*
 * core/x86_64/isrs.S - GENERATED BY isrs.sh DO NOT EDIT THIS FILE
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
 * @brief Interrupt Controller specific Interrupt Service Routines
 */
#include <errno.h>
#include <syscall.h>
#include <limits.h>
#include "isr.h"
#include "ccb.h"
#include "tcb.h"

.global isr_inithw
.global isr_exc00divzero
.global isr_irq0
.global isr_enableirq
.global isr_disableirq
.global isr_next

.extern isr_ticks
.extern isr_gainentropy
.extern isr_savecontext
.extern isr_loadcontext
.extern isr_syscall0
.extern isr_alarm
.extern isr_timer
.extern gdt64_tss
.extern excabort
.extern pmm
.extern sys_fault

.section .data
    .align	16
idt64:
    .word	(32+$numirq)*16-1
    .quad	0
    .align	8
isr_next:
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
	    movl	%edi, %ecx
	    andw	\$0xF, %cx
	    cmpb	\$8, %cl
	    jae		1f
	    inb		\$PIC_MASTER_DATA, %al
	    btrw	%cx, %ax
	    outb	%al, \$PIC_MASTER_DATA
	    ret
	1:  subb	\$8, %cl
	    inb		\$PIC_SLAVE_DATA, %al
	    btrw	%cx, %ax
	    outb	%al, \$PIC_SLAVE_DATA
	    ret

	/* void isr_disableirq(uint64_t irq) */
	isr_disableirq:
	    cmpl	\$2, %edi
	    je		1f
	    movl	%edi, %ecx
	    andw	\$0xF, %cx
	    cmpl	\$8, %edi
	    jae		2f
	    inb		\$PIC_MASTER_DATA, %al
	    btsw	%cx, %ax
	    outb	%al, \$PIC_MASTER_DATA
	1:  ret
	2:  subl	\$8, %ecx
	    inb		\$PIC_SLAVE_DATA, %al
	    btsw	%cx, %ax
	    outb	%al, \$PIC_SLAVE_DATA
	    ret

	EOF
else
	cat >>isrs.S <<-EOF
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
/* set up gates and msrs, enable interrupt controller */
isr_inithw:
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
    xorl	%eax, %eax
    movl	\$0x00130008, %edx
    wrmsr
    /* LSTAR */
    incl	%ecx
    movq	\$isr_syscall0, %rax
    movq    %rax, %rdx
    shrq    \$32, %rdx
    wrmsr
    /* SFMASK */
    incl	%ecx
    incl	%ecx
    xorl    %eax, %eax
    wrmsr
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
	    outb	%al, \$PIC_MASTER
	EOF
	read -r -d '' EOI2 <<-EOF
	    /* PIC EOI */
	    movb	\$0x20, %al
	    outb	%al, \$PIC_SLAVE
	    outb	%al, \$PIC_MASTER
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
	    addq	\$__PAGESIZE, pmm + 40

	    /* setup IRQs, mask them all */
	    xorq	%rdi, %rdi
	1:  call	isr_disableirq
	    incl	%edi
	    cmpl	\$$numirq, %edi
	    jne		1b

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
		    addq	\$__PAGESIZE, pmm + 40
		    /* setup */
		    mov		apic, %rbx
		    movl	\$0xFFFFFFFF, APIC_DFR(%rbx)
		    movl	APIC_DFR(%rbx), %eax
		    andl	\$0x00FFFFFF, %eax
		    orb		\$1, %al
		    movl	%eax, APIC_LDR(%rbx)
		    movl	\$APIC_DISABLE, APIC_LVT_TMR(%rbx)
		    movl	\$APIC_DISABLE, APIC_LVT_LINT0(%rbx)
		    movl	\$APIC_DISABLE, APIC_LVT_LINT1(%rbx)
		    movl	\$IOAPIC_NMI, APIC_LVT_PERF(%rbx)
		    movl	\$0, APIC_TASKPRI(%rbx)
		    /* enable */
		    movl	\$0x1B, %ecx
		    rdmsr
		    btsl	\$11, %eax
		    wrmsr
		    /* sw enable
		    movl	\$APIC_SW_ENABLE+39, APIC_SPURIOUS(%rbx) */
		EOF
		read -r -d '' EOI <<-EOF
		    /* APIC EOI */
		    movq	apic, %rax
		    movl	\$0, APIC_EOI(%rax)
		EOF
	fi
fi

cat >>isrs.S <<EOF
	    /* setup IRQs, mask them all */
	    xorq	%rdi, %rdi
	1:  call	isr_disableirq
	    incl	%edi
	    cmpl	\$$numirq, %edi
	    jne		1b
    /* enable NMI */
    inb		\$0x70, %al
    btrw	\$8, %ax
    outb	%al, \$0x70
    ret

/* exception handler ISRs */
	.align $isrirqmax, 0x90
EOF
i=0
for isr in $exceptions
do
	echo "			$isr" >&2
	handler=`grep " $isr" isr.c vmm.c |cut -d '(' -f 1|cut -d ' ' -f 2`
	if [ "$handler" == "" ]; then
		handler="excabort"
	fi
	if [ $i -ge 10 -a $i -le 14 ]; then
		read -r -d '' EXCERR <<-EOF
		    /* tcb->excerr = errcode; */
		    popq	tcb_excerr
		EOF
	else
		read -r -d '' EXCERR <<-EOF
		    /* tcb->excerr = 0; */
		    movq	\$0, tcb_excerr
		EOF
	fi
	if [ $i -eq 1 -o $i -eq 3 -o $i -eq 14 -o "$DEBUG" == "1" ]; then
		read -r -d '' CORECHK <<-EOF
		    /* fault in core */
		    cmpq    \$CORE_ADDRESS, (%rsp)
		    jb      1f
		    movb    \$$i, sys_fault
		    movq    \$-8, %rax
		    movq    %rax, %rdi
		    iretq
		1:
		EOF
	else
		CORECHK=""
	fi
	if [ $i -eq 13 -a "$DEBUG" == "1" ]; then
		read -r -d '' SIMIO <<-EOF
		    /* simulate serial in / out for dbg_putchar() */
		    cmpq    \$TEXT_ADDRESS, (%rsp)
		    jb      2f
		    cmpw    \$0x3f8, %dx
		    jb      2f
		    cmpw    \$0x3ff, %dx
		    ja      2f
		    movq    (%rsp), %rax
		    cmpb    \$0xEC, (%rax)
		    jne     1f
		    inb     %dx, %al
		    incq    (%rsp)
		    iretq
		1:  cmpb    \$0xEE, (%rax)
		    jne     2f
		    outb    %al, %dx
		    incq    (%rsp)
		    iretq
		2:
		EOF
	else
		SIMIO=""
	fi
	ist=1;
	if [ $i -eq 1 -o $i -eq 3 ]; then
		ist=3;
	fi
	if [ $i -eq 2 -o $i -eq 8 ]; then
		ist=2;
	fi
	cat >>isrs.S <<-EOF
	isr_$isr:
	    cli
	    $EXCERR
	    $CORECHK
	    $SIMIO
	    $DBG
	    callq	isr_savecontext
	    subq	\$$isrstack, ccb + ccb_ist$ist
	    xorq	%rdi, %rdi
	    movq	(%rsp), %rsi
	    movq	%rsp, %rdx
	    movb	\$$i, %dil
	    callq	$handler
	    addq	\$$isrstack, ccb + ccb_ist$ist
	    callq	isr_loadcontext
	    iretq
	.align $isrexcmax, 0x90

	EOF
	export i=$[$i+1];
done
cat >>isrs.S <<EOF
/* IRQ handler ISRs */
    .align $isrisrmax, 0x90
EOF
for isr in `seq 0 $numirq`
do
	echo "			irq$isr" >&2
	if [ "$EOI2" != "" -a $isr -gt 7 ]
	then
		EOI="$EOI2";
	fi
	if [ "$isr" == "$isrtimer" ]
	then
		read -r -d '' TIMER <<-EOF
		    /* isr_timer(SRV_core, SYS_IRQ, ISR_IRQTMR, 0,0,0,0); */
		    call	isr_timer
		    jmp		2f
		EOF
	else
		TIMER="";
	fi
	cat >>isrs.S <<-EOF
	.align $isrirqmax, 0x90
	isr_irq$isr:
	    cli
	    call	isr_savecontext
	    subq	\$$isrstack, ccb + ccb_ist1
	    $TIMER
	    /* tcb->memroot == sys_mapping? */
	    movq	sys_mapping, %rax
	    cmpq	%rax, tcb_memroot
	    je		1f
	    /* no, switch to system task */
	    movq	%rax, %cr3
	1:  /* isr_disableirq(irq); */
	    xorq	%rdi, %rdi
	#if DEBUG
	    movq    %rdi, dbg_lastrip
	#endif
	    movb	\$$isr, %dil
	    pushq   %rdi
	    call	isr_disableirq
	    /* ksend(EVT_DEST(SRV_CORE) | SYS_IRQ, irq, 0,0,0,0,0); */
	    popq    %rdx
	    xorq	%rcx, %rcx
	    xorq	%rsi, %rsi
	    movq	\$MQ_ADDRESS, %rdi
	    call	ksend
	2:  call	isr_gainentropy
	    $EOI
	    addq	\$$isrstack, ccb + ccb_ist1
	    call	isr_loadcontext
	    iretq
	.align $isrirqmax, 0x90

	EOF
done
