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
.type idle, STT_FUNC
.global kpanic
.type kpanic, STT_FUNC
.global kmemcpy
.type kmemcpy, STT_FUNC
.global kmemset
.type kmemset, STT_FUNC
.global kmemcmp
.type kmemcmp, STT_FUNC
.global kstrcpy
.type kstrcpy, STT_FUNC
.global kstrlen
.type kstrlen, STT_FUNC
.global kstrcmp
.type kstrcmp, STT_FUNC
.global ksend
.type ksend, STT_FUNC
.global kentropy
.type kentropy, STT_FUNC
.global klockacquire
.type klockacquire, STT_FUNC
.global klockrelease
.type klockrelease, STT_FUNC
.global vmm_map
.type vmm_map, STT_FUNC
.global vmm_init
.type vmm_init, STT_FUNC
.global vmm_mq
.type vmm_mq, STT_FUNC
.global vmm_buf
.type vmm_buf, STT_FUNC
.global vmm_getpte
.type vmm_getpte, STT_FUNC

/* idle task's code */
idle:
    sti
    hlt
    jmp     idle

/* acquire a lock, for multi core system */
klockacquire:
1:  lock
    btsq    %rdi, locks
    jnc     1f
    pause
    jmp     1b
1:  ret

/* release a lock */
klockrelease:
    lock
    btrq    %rdi, locks
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
#if DEBUG
    movb    $10, %dil
    call    dbg_putchar
#endif
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
    movb    $10, %dil
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
    movb    $27, %dil
    call    dbg_putchar
    movb    $0x5b, %dil
    call    dbg_putchar
    movb    $0x37, %dil
    call    dbg_putchar
    movb    $0x6d, %dil
    call    dbg_putchar
#endif
    movq    $54, %rdi
    movq    $5, %rsi
    call    kprintf_center
    movl    $0x9c3c1b, %edi
    call    kprintf_fg
    movl    $0x100000, %edi
    call    kprintf_bg
    movq    $kpanicsuffix, %rdi
    call    kprintf
    movl    $0x500000, %edi
    call    kprintf_fg
    movq    $kpanicsuffix2, %rdi
    call    kprintf
#if DEBUG
    movb    $27, %dil
    call    dbg_putchar
    movb    $0x5b, %dil
    call    dbg_putchar
    movb    $0x30, %dil
    call    dbg_putchar
    movb    $0x6d, %dil
    call    dbg_putchar
#endif
    addl    $46, kx
    subl    $7, ky
    call    kprintf_putlogo
    /* wait for user input */
    call    platform_waitkey
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
//    cmpq    $30, %rdx
//    ja      3f
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
// TODO: use SSE2 copy
3:
    ret

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
vmm_map:
    /* these indeces on vmm_tmp must match link.ld */
    /* shortcut for tmpmap. It's often called from sched_get_tcb */
    cmpq    $tmpmap, %rdi
    jne     1f
    movq    vmm_tmp, %rax
    addq    $24, %rax
    jmp     2f
    /* shortcut for tmp2map */
1:  cmpq    $tmp2map, %rdi
    jne     1f
    movq    vmm_tmp, %rax
    addq    $32, %rax
    jmp     2f
    /* shortcut for tmpalarm */
1:  cmpq    $tmpalarm, %rdi
    jne     1f
    movq    vmm_tmp, %rax
    addq    $40, %rax
    jmp     2f
    /* shortcut for tmpfx */
1:  cmpq    $tmpfx, %rdi
    jne     1f
    movq    vmm_tmp, %rax
    addq    $48, %rax
    jmp     2f
1:  call    vmm_getpte
2:  movq    $0x0000FFFFFFFFF000, %rbx
    andq    %rbx, %rsi
    mov     %dl, %sil
    /* copy W bit to NX bit */
    andb    $2, %dl
    shlq    $PG_NX_BIT-1, %rdx
    addq    %rdx, %rsi
    movq    %rsi, (%rax)
    invlpg  (%rdi)
    ret

vmm_init:
    push    %rbx
    push    %rcx
    push    %rdx
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
    movb    $PG_CORE, (%rax)
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
    movb    $PG_CORE, (%rbx)
    /* PDE */
    movq    (%rbx), %rax
    xorb    %al, %al
    movq    %rcx, %rbx
    shrq    $12+9, %rbx
    andl    $0x1FF, %ebx
    shlq    $3, %rbx
    add     %rax, %rbx
    movb    $PG_CORE, (%rbx)
    /* save TMPQ_ADDRESS mapping pointer */
    movb    $PG_CORE_NOCACHE, %al
    movq    %rax, %r8
    /* PTE */
    movq    (%rbx), %rax
    xorb    %al, %al
    // set bootboot and environment NX
    btsq    $PG_NX_BIT, (%rax)
    btsq    $PG_NX_BIT, 8(%rax)
    // set stack NX and supervisor page
    btsq    $PG_NX_BIT, 0xff8(%rax)
    movb    $PG_CORE_NOCACHE, 0xff8(%rax)
    push    %rax
    movq    $__data, %rdx
    andq    $0xFFFFF, %rdx
    shrq    $12, %rdx
    shlq    $3, %rdx
    addq    %rax, %rdx
    // set supervisor page attributes
1:  movb    $PG_CORE_RO, (%rax)
    btrq    $PG_NX_BIT, (%rax)
    addq    $8, %rax
    cmpq    %rdx, %rax
    jb      1b
    // no cache and exec for data and bss segments
1:  movb    $PG_CORE_NOCACHE, (%rax)
    btsq    $PG_NX_BIT, (%rax)
    addq    $8, %rax
    cmpb    $0, (%rax)
    jnz     1b
    pop     %rax
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
    subq    $8, %rbx
    mov     %r8, (%rbx)
    /* record pointer at vmm_tmp */
    subq    $8, %rbx
    andq    $0x0FFF, %rbx
    addq    %rcx, %rbx
    movq    %rbx, %rax
    popq    %rdx
    popq    %rcx
    popq    %rbx
    ret

/* map a message queue temporarily
   IN rdi: tcb->memroot */
vmm_mq:
    movq    %rdi, %rdx
    movq    $MQ_ADDRESS, %rdi
    call    vmm_getpte2
    movb    $PG_CORE_NOCACHE, %r12b
    movq    $tmpmqctrl, %rax
    addq    $__PAGESIZE-24, %rax
    movq    %r12, (%rax)
    movw    nrmqmax, %cx
    movq    $MQ_ADDRESS, %rax
    shrq    $12, %rax   // /__PAGESIZE
    addw    %ax, %cx
    movq    $TMPQ_ADDRESS, %rax
1:  invlpg  (%rax)
    addq    $__PAGESIZE, %rax
    decw    %cx
    jnz     1b

    movq    $tmp2map, %rdi
    movq    %rdx, %rsi
    movb    $PG_CORE_NOCACHE, %sil
    call    vmm_map

    ret

/* map a buffer temporarily (up to 2M)
   IN: rdi: tcb->memroot, rsi: pointer, rdx: size */
vmm_buf:
    andq    $__SLOTSIZE-1, %rdx
    push    %rdx
    push    %rsi
    movq    %rdi, %rdx
    movq    %rsi, %rdi
    push    %rdx
    push    %rdi
    call    vmm_getpte2
    movb    $PG_CORE_NOCACHE, %r12b
    movq    $tmpmqctrl, %rax
    addq    $__PAGESIZE-24, %rax
    movq    %r12, (%rax)
    pop     %rdi
    pop     %rdx
    // does buffer expand to the next slot?
    mov     (%rsp), %rax
    addq    %rsi, %rax
    shrq    $21, %rax
    movq    %rsi, %rbx
    shrq    $21, %rbx
    // buf/__SLOTSIZE == (buf+size)/__SLOTSIZE?
    cmpq    %rax, %rbx
    je      1f
    // map the next slot too
    addq    $__SLOTSIZE, %rsi
    call    vmm_getpte2
    movb    $PG_CORE_NOCACHE, %r12b
    movq    $tmpmqctrl, %rax
    addq    $__PAGESIZE-16, %rax
    movq    %r12, (%rax)
    // invalidate newly mapped buffer
1:  pop     %rax
    andq    $__SLOTSIZE-1, %rax
    addq    $TMPQ_ADDRESS, %rax
    pop     %rcx
    shrq    $12, %rcx
1:  invlpg  (%rax)
    addq    $__PAGESIZE, %rax
    decw    %cx
    jnz     1b
    ret

/* IN rdi: virtual address
 * OUT rax: address of page entry in PT */
vmm_getpte:
    movq    %cr3, %rdx
vmm_getpte2:
    /* we have tmpctrl, so we can map PTE */
    movq    vmm_tmp, %r10
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
