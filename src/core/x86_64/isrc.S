/*
 * core/x86_64/isrc.S
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
 * @brief Interrupt Ccontroller independent, common assembly routines
 */
#define _AS 1
#include <errno.h>
#include <syscall.h>
#include <limits.h>
#include "isr.h"
#include "platform.h"
#include "ccb.h"
#include "tcb.h"

.global isr_savecontext
.global isr_loadcontext
.global isr_gainentropy
.global isr_syscall0
.global isr_alarm

.extern isr_ticks
.extern isr_entropy
.extern alarmstep
.extern alarmstepmax
.extern sched_get_tcb
.extern sched_awake
.extern ccb

.section .text

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

/* shuffle bits of random seed */
isr_gainentropy:
    /* isr_entropy[isr_ticks[0]&3] ^=
       (isr_entropy[(isr_ticks[0]+1)&3])<<(isr_ticks&7) */
    movq	isr_ticks, %rax
    movq	%rax, %rdx
    incq	%rdx
    movq	%rdx, %rcx
    andq	$3, %rdx
    shlq	$3, %rdx
    andq	$3, %rax
    shlq	$3, %rax
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
    /* make the environment look like in an irq handler */
    /* tcb->rip */
    movq	%rcx, __PAGESIZE-40
    /* tcp->rsp, save stack pointer and switch to core stack */
    movq	%rsp, __PAGESIZE-16
    movq	$__PAGESIZE-40, %rsp

    /* 'send' */
    cmpl	$0x646E6573, %eax
    jne		1f
    /* if destination is SRV_core? */
    orq		%rdi, %rdi
    jnz		4f

    /*** public syscalls ***/
    /* shortcut to seterr syscall */
    cmpb	$SYS_seterr, %sil
    jne 	2f
    /* tcb->errno = rdx */
    movq	%rdx, 672
    jmp		9f
2:
    /*** "SYS" task only ***/
    /* tcb->memroot == sys_mapping? */
    movq	sys_mapping, %rcx
    cmpq	%rcx, 648
    jne		3f

    /* shortcut to ack syscall */
    cmpb	$SYS_ack, %sil
    jne     3f
    /* drivers initialized, IRQs enabled acknowledge */
    cmpl    $0xB0070E01, %edx
    jne     2f
    /* tcb->rflags |= 0x200; turn on interrupt flag */
    orl		$0x200, __PAGESIZE-24
    jmp     9f
    /* enable the given irq */
2:  movq    %rdx, %rdi
    call    isr_enableirq
    jmp     9f

3:  call	isr_syscall
    jmp		9f
4:  call	msg_sends
    jmp		9f

    /* 'call' */
1:  cmpl	$0x6c6c6163, %eax
    jne		1f
    /* tcb->gpr */
    call	isr_savecontext
    xorq	%rdi, %rdi
    call	sched_block
    call	isr_loadcontext
    /* if destination is SRV_core */
    orq		%rdi, %rdi
    jnz		2f
    call	isr_syscall
    jmp		8f
2:  call	msg_sends
    jmp		8f

    /* 'recv' */
1:  cmpl	$0x76636572, %eax
    jne		1f
    /* tcb->gpr */
    call	isr_savecontext
    /* never block system thread */
    movq	sys_mapping, %rcx
    /* tcb->memroot == sys_mapping? */
    cmpq	%rcx, 648
    je		8f
    xorq	%rdi, %rdi
    call	sched_block
    jmp		8f
    /* tcb->errno = EINVAL */
1:  movq	$EINVAL, 672
8:  call	isr_loadcontext
9:  movq	__PAGESIZE-24, %r11
    movq	__PAGESIZE-40, %rcx
    movq	__PAGESIZE-16, %rsp
    sysretq

/* check timer queue's head against timestamp and awake thread if
   time passed */
isr_alarm:
    /* isr_ticks[TICKS_NTS] += alarmstep; */
    movq    alarmstep, %rax
    addq    isr_ticks + 40, %rax
    cmpq    alarmstepmax, %rax
    jbe     1f
    /* isr_ticks[TICKS_TS]++ */
    incq    isr_ticks + 32
    /* isr_ticks[TICKS_NTS] = 0 */
    xor     %rax, %rax
1:  movq    %rax, isr_ticks + 40

    /* ccb.hd_timerq != 0? */
    movq    ccb + 0x6A, %rdi
    orq     %rdi, %rdi
    jz      1f
    call    sched_get_tcb
    /* tcb->alarmsec <= isr_ticks[TICKS_TS]? */
    movq    728(%rax), %rbx
    cmpq    isr_ticks + 32, %rbx
    jb      1f
    /* tcb->alarmns <= isr_ticks[TICKS_NTS]? */
    movq    736(%rax), %rbx
    cmpq    isr_ticks + 40, %rbx
    jb      1f
    /* ccb.hd_timerq = tcb->alarm; */
    movq    720(%rax), %rbx
    movq    %rbx, ccb + 0x6A
    /* sched_awake(tcb); */
    movq    %rax, %rdi
    call    sched_awake
1:  ret
