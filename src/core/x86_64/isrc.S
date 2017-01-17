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
#include <errno.h>
#include <syscall.h>
#include <limits.h>
#include "isr.h"
#include "ccb.h"
#include "tcb.h"

.global isr_savecontext
.global isr_loadcontext
.global isr_gainentropy
.global isr_syscall0
.global isr_alarm
.global isr_timer

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

/* restore thread's context from Thread Control Block */
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

/* shuffle bits of random seed */
isr_gainentropy:
    /* isr_entropy[isr_ticks[0]&3] ^=
       (isr_entropy[(isr_ticks[0]+1)&3])<<(isr_ticks&7) */
    movq    isr_ticks, %rax
    movq    %rax, %rdx
    incq    %rdx
    movq    %rdx, %rcx
    andq    $3, %rdx
    shlq    $3, %rdx
    andq    $3, %rax
    shlq    $3, %rax
    addq    $isr_entropy, %rax
    addq    $isr_entropy, %rdx
    andb    $0x3f, %cl
    rolq    %cl, (%rax)
    movq    (%rax), %rax
    xorq    %rax, (%rdx)
    ret

/* syscall dispatcher, platform dependent wrapper */
.align  16, 0x90
isr_syscall0:
    cli
    /* make the environment look like in an irq handler */
    /* tcb->rip */
    movq    %rcx, __PAGESIZE-40
    movq    %r10, %rcx
    /* tcp->rsp, save stack pointer and switch to core stack */
    movq    %rsp, __PAGESIZE-16
    movq    safestack, %rsp
    addq    512, %rsp
//movq $__PAGESIZE-40, %rsp

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
    movw    %si, tcb_errno
    jmp     9f
2:
    /* shortcut for yield */
    cmpb    $SYS_sched_yield, %dil
    je      7f
2:
    /*** "SYS" task only ***/
    /* tcb->memroot == sys_mapping? */
    movq    sys_mapping, %rcx
    cmpq    %rcx, tcb_memroot
    jne     3f

    /* shortcut to ack syscall */
    cmpb    $SYS_ack, %dil
    jne     3f
    /* drivers initialized, IRQs enabled acknowledge */
    cmpl    $0xB0070E01, %esi
    jne     2f
    /* tcb->rflags |= 0x200; turn on interrupt flag */
    orl     $0x200, __PAGESIZE-24
    jmp     9f
    /* enable the given irq */
2:  movq    %rsi, %rdi
    call    isr_enableirq
    cmpq    $0, isr_next
    jnz     7f
    jmp     9f

3:  call    isr_syscall
    jmp     9f
4:  call    msg_sends
    jmp     8f

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
#if DEBUG
    movq    %rax, dbg_lastrip
#endif
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
9:  movq    __PAGESIZE-24, %r11
    movq    __PAGESIZE-40, %rcx
    movq    __PAGESIZE-16, %rsp
    sysretq

isr_timer:
    /* uint128_t isr_ticks[TICKS_LO]++ */
    addq    $1, isr_ticks+16
    adcq    $0, isr_ticks+24

    /* isr_ticks[TICKS_NTS] += alarmstep; */
    movq    alarmstep, %rax
    addq    %rax, isr_ticks + 8

    /* isr_ticks[TICKS_SEC]--; */
    decq    isr_ticks + 32
    /* a sec passed, isr_ticks[TICKS_SEC]==0? */
    xorq    %rax, %rax
    cmpq    %rax, isr_ticks + 32
    jnz     1f
    /* isr_ticks[TICKS_TS]++; isr_ticks[TICKS_NTS]=0; */
    incq    isr_ticks
    movq    %rax, isr_ticks + 8
    /* isr_ticks[TICKS_SEC] = freq; */
    movq    freq, %rax
    movq    %rax, isr_ticks + 32
    /* isr_ticks[TICKS_QUANTUM] = quantumdiv; */
    movq    quantumdiv, %rax
    movq    %rax, isr_ticks + 40
    /* isr_lastfps = isr_currfps */
    movq    isr_currfps, %rax
    movq    %rax, isr_lastfps
    xorq    %rax, %rax
    movq    %rax, isr_currfps
#if DEBUG
    call    kprintf_putfps
#endif
1:
    /* check timer queue's head and awake thread if time passed */
    /* ccb.hd_timerq != 0? */
    movq    ccb + 0x6A, %rax
    orq     %rax, %rax
    jz      1f
    /* has a pid, it's tcb is mapped at tmpalarm then */
    movq    $tmpalarm, %rdi
    /* tcb->alarmsec <= isr_ticks[TICKS_TS]? */
    movq    728(%rdi), %rax
    cmpq    isr_ticks, %rax
    jb      1f
    /* tcb->alarmns <= isr_ticks[TICKS_NTS]? */
    movq    736(%rdi), %rax
    cmpq    isr_ticks + 8, %rax
    jb      1f
    movq    $tmpalarm, %rdi
    /* isr_next = sched_awake(tcb); */
    call    sched_awake
    /* ccb.hd_timerq = tcb->alarm; */
    movq    $tmpalarm, %rdi
    movq    720(%rdi), %rsi
    movq    %rsi, ccb + 0x6A
    /* tcb->alarm != 0? */
    orq     %rsi, %rsi
    jz      1f
    /* map next alarm thread, kmap(&tmpalarm, tcb->alarm, PG_CORE_NOCACHE); */
    xorq    %rdx, %rdx
    movw    $PG_CORE_NOCACHE, %dx
    call    kmap
1:
    /* isr_ticks[TICKS_QUANTUM]--; */
    decq    isr_ticks + 40
    cmpq    $0, isr_ticks + 40
    jnz     2f
    /* isr_ticks[TICKS_QALL]++; */
    incq    isr_ticks + 56
    /* isr_ticks[TICKS_QUANTUM] = quantumdiv; */
    movq    quantumdiv, %rax
    movq    %rax, isr_ticks + 40
    /* if(tcb->rip < 0) tcb->syscnt++; else tcb->billcnt++; */
    movq    $656, %rbx
    movb    __PAGESIZE-33, %al
    cmpb    $0xFF, %al
    jne     1f
    addq    $8, %rbx
1:  incq    (%rbx)
    /* switch to a new thread if any */
    call    sched_pick
2:
    /* isr_ticks[TICKS_FPS]--; */
    decq    isr_ticks + 48
    cmpq    $0, isr_ticks + 48
    jnz     1f
    mov     fpsdiv, %rax
    movq    %rax, isr_ticks+48
    /* build and send a mem2vid message */
    /* switch to system task */
    movq    sys_mapping, %rax
    movq    %rax, %cr3
    /* msg_sends(EVT_DEST(SRV_CORE) | SYS_swapbuf, 0,0,0,0,0); */
    xorq    %rsi, %rsi
#if DEBUG
    movq    %rsi, dbg_lastrip
#endif
    movb    $SYS_swapbuf, %sil
    movq    $MQ_ADDRESS, %rdi
    call    ksend
1:
    ret
