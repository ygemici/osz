/*
 * core/x86_64/libk.S
 *
 * Copyright 2017 CC-by-nc-sa bztsrc@github
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
 * @brief low level kernel routines for the core
 */

#define _AS 1
#include "arch.h"

.section .text
.global idle
.global kpanic
.global kmemcpy
.global kmemset
.global kmemcmp
.global kmap
.global kmap_init
.global kmap_mq
.global kmap_getpte
.global kstrcpy
.global kstrlen
.global kstrcmp
.global ksend
.global kwaitkey
.global kentropy
.global klockacquire
.global klockrelease

/* idle task's code */
idle:
    sti
    hlt
    jmp     idle

/* acquire a lock, for multi core system */
klockacquire:
1:  lock
    btsq    %rdi, multicorelock
    jnc     1f
    pause
    jmp     1b
1:  ret

/* release a lock */
klockrelease:
    lock
    btrq    %rdi, multicorelock
    ret

/* shuffle bits of random seed */
kentropy:
    pushq   %rdi
    pushq   %rsi
    movq    $srand, %rdi
    movq    %rdi, %rax
    movq    %rax, %rdx
    incq    %rdx
    movq    %rdx, %rcx
    andq    $3, %rdx
    shlq    $3, %rdx
    andq    $3, %rax
    shlq    $3, %rax
    addq    %rdi, %rax
    addq    %rdi, %rdx
    andb    $0x3f, %cl
    rolq    %cl, (%rax)
    pushq   %rdx
    movq    (%rax), %rsi
    xorq    %rsi, (%rdx)
    movq    $16807, %rcx
    movq    (%rdx), %rax
    mulq    %rcx
    movq    bootboot + 24, %rsi
    xorq    %rsi, %rdx
    movq    $srand, %rsi
    andq    $24-1, %rdx
    addq    %rdx, %rsi
    movq    %rax, (%rsi)
    popq    %rdi
    stosq
    movq    ticks, %rsi
    andq    $24-1, %rsi
    addq    $srand, %rsi
    movq    (%rsi), %rax
    mulq    %rcx
    movq    %rax, (%rsi)
    movq    $srand, %rsi
    movq    (%rsi), %rax
    xorq    %rax, 8(%rsi)
    movq    8(%rsi), %rax
    xorq    %rax, 16(%rsi)
    movq    16(%rsi), %rax
    xorq    %rax, 24(%rsi)
    movq    24(%rsi), %rax
    xorq    %rax, (%rsi)
    popq    %rsi
    popq    %rdi
    ret

/* kernel panic */
kpanic:
    movq    %rsp, %rbp
    pushq   %rdi
    pushq   %rsi
    pushq   %rdx
    call    kprintf_fade
    /* clear the first line */
    movl    $0xFFDD33, fg
    movl    $0, bg
    movq    $kpanicprefix, %rdi
    call    kprintf
    popq    %rdx
    popq    %rsi
    popq    %rdi
    addq    $8, %rsp
    call    kprintf
    movq    __PAGESIZE-40, %rsi
    /* canonized address in rip? */
    movq    %rsi, %rax
    orq     %rax, %rax
    jz      2f
    shrq    $48, %rax
    cmpw    $0xFFFF, %ax
    je      1f
    orw     %ax, %ax
    jnz     2f
    /* if so, print rip and try to resolve symbol for it */
1:  movq    $kpanicrip, %rdi
    call    kprintf
    movq    __PAGESIZE-40, %rdi
    call    elf_sym
    mov     %rax, %rsi
    movq    $kpanicsym, %rdi
    call    kprintf
2:
#if DEBUG
    movb    $13, %dil
    call    dbg_putchar
    movb    $10, %dil
    call    dbg_putchar
    cmpb    $0, dbg_enabled
    jz      1f
    movq    $theme_panic, dbg_theme
    movq    __PAGESIZE-40, %rdi
    movq    %rbp, %rsi
    xorq    %rdx, %rdx
    call    dbg_enable
    jmp     2f
1:
#endif
    movq    $54, %rdi
    movq    $5, %rsi
    call    kprintf_center
    push    kx
    movl    $0x9c3c1b, fg
    movl    $0x100000, bg
    movq    $kpanicsuffix, %rdi
    call    kprintf
    movl    $0x500000, fg
    movq    $kpanicsuffix2, %rdi
    call    kprintf
    pop     kx
    addl    $46, kx
    addl    $1, ky
    call    kprintf_putlogo
    /* wait for user input */
1:  call    kwaitkey
    /* don't accept Alt key. I press it when I press Alt+Q or Alt+F4 to close qemu and bochs :-) */
    cmpb    $0x38, %al
    je      1b
    /* reboot computer */
2:  call    kprintf_reboot
    cli
    hlt

/* misc memory functions */
kmemcpy:
    cld
    orq     %rdi, %rdi
    jz      2f
    orq     %rsi, %rsi
    jz      2f
    orq     %rdx, %rdx
    jz      2f
1:  movq    %rdx, %rcx
    shrq    $3, %rcx
    or      %rcx, %rcx
    jz      1f
    repnz   movsq
1:  movb    %dl, %cl
    andb    $0x7, %cl
    repnz   movsb
    cld
2:  ret

kmemset:
    cld
    orq     %rdx, %rdx
    jz      2f
    orq     %rdi, %rdi
    jz      2f
    movq    %rdx, %rcx
    movq    %rdi, %rbx
    movb    %sil, %al
    shrq    $3, %rcx
    orq     %rcx, %rcx
    jz      1f
    movb    %al, %ah
    shlq    $16, %rax
    movb    %sil, %al
    movb    %al, %ah
    shlq    $16, %rax
    movb    %sil, %al
    movb    %al, %ah
    shlq    $16, %rax
    movb    %sil, %al
    movb    %al, %ah
    repnz   stosq
1:  movb    %dl, %cl
    andb    $0x7, %cl
    repnz   stosb
    movq    %rbx, %rax
2:  ret

kmemcmp:
    movb    $0, sys_fault
    pushq   %rdi
    xorq    %rax, %rax
    incl    %edx
1:  decl    %edx
    jz      3f
    lodsb
    cmpb    $0, sys_fault
    jnz     3f
    or      %al, %al
    jnz     2f
    cmpb    $0, (%rdi)
    je      3f
    cmpb    $0, sys_fault
    jnz     3f
    incb    %al
    jmp     3f
2:  subb    (%rdi), %al
    incq    %rdi
    or      %al, %al
    jz      1b
3:  popq    %rdi
    ret

kstrcpy:
1:  lodsb
    stosb
    or      %al, %al
    jnz     1b
    movq    %rdi, %rax
    ret

kstrlen:
    movq    %rdi, %rsi
    xor     %rcx, %rcx
    decq    %rcx
1:  lodsb
    incq    %rcx
    or      %al, %al
    jnz     1b
    movq    %rcx, %rax
    ret

kstrcmp:
    xorq    %rax, %rax
1:  lodsb
    cmpb    %al, (%rdi)
    jne     2f
    incq    %rdi
    or      %al, %al
    jnz     1b
2:  ret

/* rdi: virtual address
   rsi: physical address
   rdx: access rights
*/
kmap:
    /* these indeces on kmap_tmp must match supervisor.ld */
    /* shortcut for tmpmap. It's often called from sched_get_tcb */
    cmpq    $tmpmap, %rdi
    jne     1f
    movq    kmap_tmp, %rax
    addq    $8, %rax
    jmp     2f
    /* shortcut for tmp2map */
1:  cmpq    $tmp2map, %rdi
    jne     1f
    movq    kmap_tmp, %rax
    addq    $32, %rax
    jmp     2f
    /* shortcut for tmpfx */
1:  cmpq    $tmpfx, %rdi
    jne     1f
    movq    kmap_tmp, %rax
    addq    $40, %rax
    jmp     2f
    /* shortcut for tmpalarm */
1:  cmpq    $tmpalarm, %rdi
    jne     1f
    movq    kmap_tmp, %rax
    addq    $48, %rax
    jmp     2f
1:  call    kmap_getpte
2:  andw    $0xF000, %si
    mov     %dl, %sil
    /* copy W bit to NX bit */
    andb    $2, %dl
    shlq    $62, %rdx
    addq    %rdx, %rsi
    movq    %rsi, (%rax)
    invlpg  (%rdi)
    ret

kmap_init:
    push    %rbx
    push    %rcx
    /* this is called very early. Relies on identity mapping
       to find the physical address of tmppte pointer in PTE */
    movq    $tmppte, %rcx
    /* PML4 */
    movq    %rcx, %rbx
    shrq    $12+9+9+9, %rbx
    andl    $0x1FF, %ebx
    shlq    $3, %rbx
    movq    %cr3, %rax
    andw    $0xF000, %ax
    movq    %rax, identity_mapping
    add     %rax, %rbx
    /* save core mapping pointer */
    addq    $__PAGESIZE-8, %rax
    movq    (%rax), %rax
    movq    %rax, core_mapping
    /* PDPE */
    movq    (%rbx), %rax
    xorb    %al, %al
    movq    %rcx, %rbx
    shrq    $12+9+9, %rbx
    andl    $0x1FF, %ebx
    shlq    $3, %rbx
    add     %rax, %rbx
    /* PDE */
    movq    (%rbx), %rax
    xorb    %al, %al
    movq    %rcx, %rbx
    shrq    $12+9, %rbx
    andl    $0x1FF, %ebx
    shlq    $3, %rbx
    add     %rax, %rbx
    /* save TMPQ_ADDRESS mapping pointer */
    movb    $PG_CORE_NOCACHE, %al
    movq    %rax, %r8
    /* PTE */
    movq    (%rbx), %rax
    xorb    %al, %al
    movq    %rcx, %rbx
    shrq    $12, %rbx
    andl    $0x1FF, %ebx
    shlq    $3, %rbx
    add     %rax, %rbx
    /* map it at tmppte */
    mov     %rbx, %rax
    addq    $8, %rax
    movq    %rax, %rdi
    andw    $0xF000, %ax
    incw    %ax
    mov     %rax, (%rbx)
    /* map TMPQ_ADDRESS pde at tmpmqctrl */
    addq    $8, %rbx
    mov     %r8, (%rbx)
    /* record pointer at kmap_tmp */
    subq    $24, %rbx
    andq    $0x0FFF, %rbx
    addq    %rcx, %rbx
    movq    %rbx, %rax
    popq    %rcx
    popq    %rbx
    ret

/* map a message queue temporairly
   IN rdi: tcb->memroot */
kmap_mq:
    movq    %rdi, %rdx
    movq    $MQ_ADDRESS, %rdi
    call    kmap_getpte2
    movb    $PG_CORE_NOCACHE, %r12b
    movq    $tmpmqctrl, %rax
    addq    $__PAGESIZE-16, %rax
    movq    %r12, (%rax)
    movw    nrmqmax, %cx
    movq    $MQ_ADDRESS / __PAGESIZE, %rax
    addw    %ax, %cx
    movq    $TMPQ_ADDRESS, %rax
1:  invlpg  (%rax)
    addq    $__PAGESIZE, %rax
    decw    %cx
    jnz     1b

    movq    $tmp2map, %rdi
    movq    %rdx, %rsi
    movb    $PG_CORE_NOCACHE, %sil
    call    kmap

    ret

/* IN rdi: virtual address
 * OUT rax: address of page entry in PT */
kmap_getpte:
    movq    %cr3, %rdx
kmap_getpte2:
    /* we have tmpctrl, so we can map PTE */
    movq    kmap_tmp, %r10
    xorq    %r9, %r9
    /* PML4 */
    movq    %rdi, %rax
    shrq    $12+9+9+9, %rax
    andl    $0x1FF, %eax
    shlq    $3, %rax
    andw    $0xF000, %dx
    incw    %dx
    movq    %rdx, (%r10)
    invlpg  tmpctrl
    addq    $tmpctrl, %rax
    incb    %r9b
    /* PDPE */
    mov     (%rax), %rdx
    movq    %rdi, %rax
    shrq    $12+9+9, %rax
    andl    $0x1FF, %eax
    shlq    $3, %rax
    andw    $0xF000, %dx
    orq     %rdx, %rdx
    jz      1f
    incw    %dx
    movq    %rdx, (%r10)
    invlpg  tmpctrl
    addq    $tmpctrl, %rax
    incb    %r9b
    /* PDE */
    mov     (%rax), %rdx
    movq    %rdi, %rax
    shrq    $12+9, %rax
    andl    $0x1FF, %eax
    shlq    $3, %rax
    andw    $0xF000, %dx
    orq     %rdx, %rdx
    jz      1f
    incw    %dx
    movq    %rdx, (%r10)
    invlpg  tmpctrl
    addq    $tmpctrl, %rax
    incb    %r9b
    /* PTE */
    mov     (%rax), %rdx
    movq    %rdi, %rax
    shrq    $12, %rax
    andl    $0x1FF, %eax
    shlq    $3, %rax
    andw    $0xF000, %dx
    orq     %rdx, %rdx
    jz      1f
    incw    %dx
    movq    %rdx, %r12
    movq    %rdx, (%r10)
    invlpg  tmpctrl
    addq    $tmpctrl, %rax
    ret
1:  movq    %r9, %rdx
    movq    %rdi, %rsi
    movq    $kpanictlb, %rdi
    call    kpanic

/* send a message to a message queue. This has to be effective
   and I didn't liked the code gcc generated, that's why it's
   asm, otherwise this routine is not platform specific.
    IN: rdi: mq, rsi: event, rdx: arg0, rcx: arg1, r8: arg2
    OUT: true on success
*/
ksend:
    pushf
    cli
    /* check if message queue is full */
    movq    (%rdi), %rax
    /* mqhdr->mq_start+1 == mqhdr->mq_end? */
    incq    %rax
    cmpq    8(%rdi), %rax
    je      1f
    /* mqhdr->mq_start+1 == mqhdr->mq_size && mqhdr->mq_end==1? */
    cmpq    16(%rdi), %rax
    jne      2f
    cmpq    $1, 8(%rdi)
    jne     2f
1:  /* mqhdr->mq_end++ */
    incq    8(%rdi)
    movq    8(%rdi), %rax
    cmpq    16(%rdi), %rax
    jne     2f
    movq    $1, 8(%rdi)
2:  /* replace destination pid with sender's */
    andq    $0xFFFF, %rsi
    /* tcb.pid<<16 */
    movq    tcb_pid, %rax
    shlq    $16, %rax
    orq     %rax, %rsi
    /* copy message */
    movq    (%rdi), %rax
    shlq    $6, %rax
    addq    %rdi, %rax
    movq    %rsi, (%rax)
    movq    %rdx, 8(%rax)
    movq    %rcx, 16(%rax)
    movq    %r8, 24(%rax)
    movq    %r9, 32(%rax)
    movq    24(%rbp), %r9
    movq    %r9, 40(%rax)
    movq    16(%rbp), %r9
    movq    %r9, 48(%rax)
    incq    tcb_serial
    movq    tcb_serial, %r9
    movq    %r9, 56(%rax)
    /* mqhdr->mq_start++ */
    incq    (%rdi)
    movq    (%rdi), %rax
    cmpq    16(%rdi), %rax
    jne     3f
    movq    $1, (%rdi)
3:  xorq    %rax, %rax
    incb    %al
    popf
    ret
4:  xorq    %rax, %rax
    popf
    ret

/* an early implementation, called by kprintf */
kwaitkey:
    xorq    %rax, %rax
    xorq    %rdx, %rdx
    /* is there a key waiting? */
1:  inb     $0x64, %al
    andb    $1, %al
    jnz     2f
    pause
#if DEBUG
    movw    $0x3fd, %dx
    inb     %dx, %al
    /* do we have a serial port at all? */
    cmpb    $0xFF, %al
    je      1b
    /* is there a received byte waiting? */
    andb    $1, %al
    jz      1b
    subb    $5, %dl
    inb     %dx, %al
    orb     %al, %al
    jz      1b
    movb    $0, dbg_iskbd
    jmp     8f
#else
    jmp     1b
#endif
    /* yes, read the scancode */
2:  inb     $0x60, %al
    orb     %al, %al
    jz      1b
    movb    %al, %dl
    /* acknowledge */
    inb     $0x61, %al
    outb    %al, $0x61
#if DEBUG
    /* handle shift key here for debugger */
    movb    $1, dbg_iskbd
    cmpb    $54, %dl
    je      2f
    cmpb    $42, %dl
    jne     1f
2:  movb    $1, dbg_isshft
    jmp     3f
1:  cmpb    $54+0x80, %dl
    je      2f
    cmpb    $42+0x80, %dl
    jne     3f
2:  movb    $0, dbg_isshft
3:
#endif
    /* check extended scancode */
    cmpb    $0xE0, %dl
    jne     1f
    movb    $1, 9f
    jmp     kwaitkey
1:  cmpb    $0xE1, %dl
    jne     1f
    movb    $2, 9f
    jmp     kwaitkey
1:  cmpb    $0xFF, %dl
    jne     1f
    movb    $3, 9f
    jmp     kwaitkey
1:  xorq    %rax, %rax
    movb    9f, %ah
    movb    $0, 9f
    movb    %dl, %al
    bt      $7, %ax
    jc      kwaitkey
8:  /* extended scancode in ax */
    ret
9:
    .byte 0
