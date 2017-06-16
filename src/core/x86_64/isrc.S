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
 * @brief Interrupt Controller independent, common assembly routines
 */
#include <errno.h>
#include <syscall.h>
#include <limits.h>
#include "isr.h"
#include "ccb.h"
#include "tcb.h"
#include "../tcb.h"

.global isr_savecontext
.global isr_loadcontext
.global isr_syscall0
.global isr_timer
.global isr_gainentropy
.global isr_tmrcalibrate

.extern alarmstep
.extern sched_awake
.extern ccb
.extern isr_next
.extern bogomips

.section .text

/**
 * store thread's context into Thread Control Block
 */
isr_savecontext:
    movq    %rax, tcb_gpr +   0
    movq    %rbx, tcb_gpr +   8
    movq    %rcx, tcb_gpr +  16
    movq    %rdx, tcb_gpr +  24
    movq    %rsi, tcb_gpr +  32
    movq    %rdi, tcb_gpr +  40
    movq    %r8,  tcb_gpr +  48
    movq    %r9,  tcb_gpr +  56
    movq    %r10, tcb_gpr +  64
    movq    %r11, tcb_gpr +  72
    movq    %r12, tcb_gpr +  80
    movq    %r13, tcb_gpr +  88
    movq    %r14, tcb_gpr +  96
    movq    %r15, tcb_gpr + 104
    movq    %rbp, tcb_gpr + 112
    ret

/**
 * restore thread's context from Thread Control Block
 */
isr_loadcontext:
    movq    tcb_gpr +   0, %rax
    movq    tcb_gpr +   8, %rbx
    movq    tcb_gpr +  16, %rcx
    movq    tcb_gpr +  24, %rdx
    movq    tcb_gpr +  32, %rsi
    movq    tcb_gpr +  40, %rdi
    movq    tcb_gpr +  48, %r8
    movq    tcb_gpr +  56, %r9
    movq    tcb_gpr +  64, %r10
    movq    tcb_gpr +  72, %r11
    movq    tcb_gpr +  80, %r12
    movq    tcb_gpr +  88, %r13
    movq    tcb_gpr +  96, %r14
    movq    tcb_gpr + 104, %r15
    movq    tcb_gpr + 112, %rbp
    ret

/**
 * syscall dispatcher, platform dependent wrapper around isr_syscall()
 * performance hacks and special access checks mostly
 */
.align  16, 0x90
isr_syscall0:
    cli
    cld
    /* make the environment look like in an irq handler */
    /* tcb->rip */
    movq    %rcx, __PAGESIZE-40
    /* use %r12 in ABI as %rcx and %r11 are clobbered */
    movq    %r12, %rcx
#if DEBUG
    /* dbg_printf */
    orl     %eax, %eax
    jnz     1f
    callq   kprintf
    jmp     6f
1:
#endif
    /* tcp->rsp, save stack pointer and switch to core stack */
    movq    %rsp, __PAGESIZE-16
    movq    safestack, %rsp
    addq    512, %rsp

    /* 'send' */
    cmpl    $0x646E6573, %eax
    jne     1f
    /* is destination SRV_CORE? */
    movq    %rdi, %rax
    shrq    $16, %rax
    orq     %rax, %rax
    jnz     4f

    /*** public syscalls ***/
    /* shortcut to seterr syscall */
    cmpb    $SYS_seterr, %dil
    jne     2f
    /* tcb->errno = rsi */
    movb    %sil, tcb_errno
    jmp     9f
2:
    /* shortcut for yield */
    cmpb    $SYS_sched_yield, %dil
    je      7f

    /* small alarm(sec,usec) */
    cmpb    $SYS_alarm, %dil
    jne     2f
    /* sec == 0? */
    cmpq    $0, %rsi
    jnz     3f
    /* usec < alarmstep? */
    cmpq    alarmstep, %rdx
    ja      3f
    /* loops = usec*bogomips/1000000 */
    movq    %rdx, %rax
    xorq    %rdx, %rdx
    movq    bogomips, %rcx
    mulq    %rcx
    movq    $1000000, %rcx
    divq    %rcx
5:  dec     %rax
    jnz     5b
    jmp     9f

    /*** drivers tasks only ***/
2:  cmpb    $PRI_DRV, tcb_priority
    jne     3f
    /* shortcut to ack syscall */
    cmpb    $SYS_ack, %dil
    jne     3f
    movq    %rsi, %rdi
    /* tcb->memroot == irq_routing_table[irq]? */
    shlq    $3, %rsi
    addq    irq_routing_table, %rsi
    movq    (%rsi), %rsi
    cmpq    tcb_memroot, %rsi
    jne     9f
    /* enable the given irq */
    call    isr_enableirq
    jmp     9f

3:  call    isr_syscall
    jmp     9f
4:  call    msg_sends
    jmp     9f

    /* 'call' */
1:  cmpl    $0x6c6c6163, %eax
    jne     1f
    /* tcb->gpr */
    call    isr_savecontext
    xorq    %rdi, %rdi
    call    sched_block
    call    sched_pick
    call    isr_loadcontext
    /* if destination is SRV_core */
    movq    %rdi, %rax
    shrq    $16, %rax
    orq     %rax, %rax
    jnz     2f
    call    isr_syscall
    jmp     7f
2:  call    msg_sends
    /* map dest thread */
7:  movq    isr_next, %rax
3:  cmpq    $__PAGESIZE, %rax
    jb      8f
    movq    %rax, %cr3
    xorq    %rax, %rax
    movq    %rax, isr_next
    jmp     8f

    /* 'recv' */
1:  cmpl    $0x76636572, %eax
    jne     1f
    /* tcb->gpr */
    call    isr_savecontext
    xorq    %rdi, %rdi
    /* sched_block(tcb); */
    call    sched_block
    /* isr_next = sched_pick(); */
    call    sched_pick
    jmp     3b

    /* tcb->errno = EINVAL */
1:  movw    $EINVAL, tcb_errno

8:  call    isr_loadcontext
    /* raise priority level on return (idle task only) */
    cmpq    $idle, __PAGESIZE-40
    jb      9f
    cmpq    $idle+4, __PAGESIZE-40
    ja      9f
    movq    $__PAGESIZE-40, %rsp
    iretq
9:  movq    __PAGESIZE-16, %rsp
6:  movq    __PAGESIZE-24, %r11
    movq    __PAGESIZE-40, %rcx
    sysretq

/**
 * calibrate busy loop used by isr_syscall0 by calculating how many times we increase a register
 * between two interrupts running at tmrfreq. See isr_irqtmrcal in isrs.S
 */
isr_tmrcalibrate:
    xorq    %rax, %rax
    xorq    %rbx, %rbx
    xorq    %rcx, %rcx
    sti
1:  incq    %rcx
    orq     %rbx, %rbx
    jz      1b
    cli
    movq    %rcx, bogomips
    subq    %rbx, %rax
    ret

/**
 * handle timers, driver independent code. Called by PIT, RTC or HPET code in isrs.S
 */
isr_timer:
/* global clocksource */
    /* uint128_t ticks[TICKS_LO]++ */
    movq    $ticks, %rbx
    addq    $1, 16(%rbx)
    adcq    $0, 24(%rbx)

    /* ticks[TICKS_NTS] += alarmstep; */
    movq    alarmstep, %rax
    addq    %rax, 8(%rbx)

    /* seccnt--; */
    decq    seccnt
    /* a sec passed, seccnt==0? */
    xorq    %rax, %rax
    cmpq    %rax, seccnt
    jnz     1f
    /* ticks[TICKS_TS]++; ticks[TICKS_NTS]=0; */
    incq    (%rbx)
    movq    %rax, 8(%rbx)
    /* seccnt = tmrfreq; */
    movq    tmrfreq, %rax
    movq    %rax, seccnt
    /* isr_lastfps = isr_currfps; isr_currfps=0; */
    movw    isr_currfps, %ax
    movw    %ax, isr_lastfps
    xorw    %ax, %ax
    movw    %ax, isr_currfps
#if DEBUG
    call    kprintf_putfps
#endif
1:

/* housekeeping */
    /* if(tcb->rip > 0) tcb->billcnt++; else tcb->syscnt++; */
    movb    __PAGESIZE-33, %al
    cmpb    $0xFF, %al
    jne     1f
    incq    tcb_billcnt+8
    jmp     9f
1:  incq    tcb_billcnt

/* preemption */
    /* never preempt real-time and driver tasks */
    cmpb    $PRI_DRV, tcb_priority
    jbe     9f

    /* check timer queue's head and awake thread if time passed */
    /* ccb.hd_timerq != 0? */
    movq    ccb + ccb_hd_timerq, %rax
    orq     %rax, %rax
    jz      1f
    /* has a pid, it's tcb is mapped at tmpalarm then */
    movq    $ticks, %rbx
    movq    $tmpalarm, %rdi
    /* tcb->alarmsec < ticks[TICKS_TS]? */
    movq    728(%rdi), %rax
    cmpq    (%rbx), %rax
    ja      1f
    /* tcb->alarmsec > ticks[TICKS_TS]? */
    jb      2f
    /* tcb->alarmns <= ticks[TICKS_NTS]? */
    movq    736(%rdi), %rax
    cmpq    8(%rbx), %rax
    ja      1f
2:  /* isr_next = sched_awake(tcb); */
    call    sched_awake
    movq    $tmpalarm, %rdi
    /* ccb.hd_timerq = tcb->alarm; */
    movq    720(%rdi), %rax
    movq    %rax, ccb + ccb_hd_timerq
    /* tcb->alarm != 0? */
    orq     %rax, %rax
    jz      3f
    movq    %rax, %rsi
    xorq    %rax, %rax
    movq    %rax, 720(%rdi)
    /* map next alarm thread, kmap(&tmpalarm, tcb->alarm*__PAGESIZE, PG_CORE_NOCACHE); */
    shlq    $12, %rsi
    xorq    %rdx, %rdx
    movw    $PG_CORE_NOCACHE, %dx
    call    kmap
    jmp     3f
1:

    /* qcnt--; */
    decq    qcnt
    /* qcnt==0? */
    xorq    %rax, %rax
    cmpq    %rax, qcnt
    jnz     1f

    /* pick the next thread to run */
    call    sched_pick

3:  /* qcnt = qdiv; */
    movq    qdiv, %rax
    movq    %rax, qcnt
9:

#if DEBUG
    /* polling serial line, check LSR */
    movw    $0x3f8+5, %dx
    inb     %dx, %al
    /* brk requested? call internal debugger */
    btw     $4, %ax
    jnc     1f
    int     $1
1:
#endif
    ret
