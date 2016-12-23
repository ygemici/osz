/*
 * core/x86_64/libk.S
 *
 * Copyright 2016 CC-by-nc-sa bztsrc@github
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
#include "tcb.h"

.section .text
.global kpanic
.global kmemcpy
.global kmemset
.global kmemcmp
.global kmemvid
.global kmap
.global kmap_init
.global kstrcpy
.global kstrlen
.global kstrcmp
.global ksend
.extern kmap_tmp
.extern tmpmap
.extern tmp2map
.extern tmppte

/* kernel panic */
kpanic:
    movl    $0xFFDD33, fg
    movl    $0x000000, bg
    pushq   %rdi
    movq    $kpanicprefix, %rdi
    call    kprintf
    popq    %rdi
    addq    $8, %rsp
    call    kprintf
    movl    $0x8c2c0b, fg
    movl    $0x100000, bg
    movq    $kpanicsuffix, %rdi
    cmpl    $0, kx
    jne     1f
    incq    %rdi
1:  call    kprintf
    movl    $0x500000, fg
    movq    $kpanicsuffix2, %rdi
    call    kprintf
    movl    $46, kx
    subl    $6, ky
    call    kprintf_putlogo
    cli
    hlt
kpanicprefix:
.asciz "OS/Z core panic: "
kpanicsuffix:
.ascii "\n                                                      \n"
.ascii "   __|  _ \\  _ \\ __|   _ \\  \\    \\ |_ _|  __|         \n"
.ascii "  (    (   |   / _|    __/ _ \\  .  |  |  (            \n"
.asciz " \\___|\\___/ _|_\\___|  _| _/  _\\_|\\_|___|\\___|         \n"
kpanicsuffix2:
.ascii "                                                      \n"
.ascii "                       KEEP CALM                      \n"
.ascii "               AND RESTART YOUR COMPUTER              \n"
.asciz "                                                      "

/* misc memory functions */
kmemcpy:
    movq    %rdx, %rcx
    repnz   movsb
    ret

kmemset:
    movb    %sil, %al
    movq    %rdx, %rcx
    repnz   stosb
    ret

kmemcmp:
    xorq    %rax, %rax
    incl    %edx
1:  decl    %edx
    jz      3f
    lodsb
    or      %al, %al
    jnz     2f
    cmpb    $0, (%rdi)
    je      3f
    incb    %al
    jmp     3f
2:  subb    (%rdi), %al
    incq    %rdi
    or      %al, %al
    jz      1b
3:  ret

kstrcpy:
1:  lodsb
    stosb
    or      %al, %al
    jnz     1b
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

kmemvid:
    ret

kmap:
    call    kmap_getpte
    andw    $0xF000, %si
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
       to find the physical address of tmpmap pointer in PTE */
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
    /* save core mappings pointer */
    addq    $4096-8, %rax
    movq    (%rax), %rax
    movq    %rax, core_mapping
    /* save core mappings pointer */
    andw    $0xF000, %ax
    addq    $4096-8, %rax
    movq    (%rax), %rax
    movq    %rax, corepde_mapping
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
    /* PTE */
    movq    (%rbx), %rax
    xorb    %al, %al
    movq    %rcx, %rbx
    shrq    $12, %rbx
    andl    $0x1FF, %ebx
    shlq    $3, %rbx
    add     %rax, %rbx
    /* map it at tmppte */
    pushq   %rdi
    mov     %rbx, %rax
    addq    $8, %rax
    movq    %rax, %rdi
    andw    $0xF000, %ax
    incw    %ax
    mov     %rax, (%rbx)
    /* clear the rest */
    pushq   %rcx
    pushq   %rax
    xorq    %rax, %rax
    movq    %rbx, %rcx
    addq    $4095, %rcx
    andw    $0xF000, %cx
    subq    %rbx, %rcx
    shrq    $3, %rcx
    dec     %rcx
    dec     %rcx
    repnz   stosq
    popq    %rax
    popq    %rcx
    popq    %rdi
    /* record pointer */
    subq    $16, %rbx
    andq    $0x0FFF, %rbx
    addq    %rcx, %rbx
    movq    %rbx, %rax
    popq    %rcx
    popq    %rbx
    ret

/* IN rdi: virtual address
 * OUT rax: address of page entry in PT */
kmap_getpte:
    /* we have at least tmpmap, so we can map PTE */
    movq    kmap_tmp, %rbx
    /* PML4 */
    movq    %rdi, %rax
    shrq    $12+9+9+9, %rax
    andl    $0x1FF, %eax
    shlq    $3, %rax
    movq    %cr3, %rdx
    andw    $0xF000, %dx
    incw    %dx
    movq    %rdx, (%rbx)
    invlpg  tmpmap
    addq    $tmpmap, %rax
    /* PDPE */
    mov     (%rax), %rdx
    movq    %rdi, %rax
    shrq    $12+9+9, %rax
    andl    $0x1FF, %eax
    shlq    $3, %rax
    andw    $0xF000, %dx
    incw    %dx
    movq    %rdx, (%rbx)
    invlpg  tmpmap
    addq    $tmpmap, %rax
    /* PDE */
    mov     (%rax), %rdx
    movq    %rdi, %rax
    shrq    $12+9, %rax
    andl    $0x1FF, %eax
    shlq    $3, %rax
    andw    $0xF000, %dx
    incw    %dx
    movq    %rdx, (%rbx)
    invlpg  (%rbx)
    invlpg  tmpmap
    addq    $tmpmap, %rax
    /* PTE */
    mov     (%rax), %rdx
    movq    %rdi, %rax
    shrq    $12, %rax
    andl    $0x1FF, %eax
    shlq    $3, %rax
    andw    $0xF000, %dx
    incw    %dx
    movq    %rdx, (%rbx)
    invlpg  tmpmap
    addq    $tmpmap, %rax
    ret

/* send a message to a message queue. This has to be effective
   and I didn't like the code gcc generated, that's why it's
   asm, otherwise this routine is not platform specific.
    IN: rdi: mq, rsi: event, rdx: arg0, rcx: arg1, r8: arg2
*/
ksend:
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
1:  /* we won't block if we're sending to system thread,
       rather remove the last item from the message queue
       to make space and we continue */
    movq    %rsi, %rax
    /* destination pid == 0? */
    shrq    $16, %rax
    orq     %rax, %rax
    jne     4f
    /* mqhdr->mq_end++ */
    incq    8(%rdi)
    movq    8(%rdi), %rax
    cmpq    16(%rdi), %rax
    jne     2f
    movq    $1, 8(%rdi)
2:  /* replace destination pid with sender's */
    andq    $0xFFFF, %rsi
    /* tcb.mypid<<16 */
    movq    $OSZ_tcb_mypid, %rax
    shlq    $16, %rax
    orq     %rax, %rsi
    /* copy message */
    movq    (%rdi), %rax
    shlq    $5, %rax
    addq    %rdi, %rax
    movq    %rsi, (%rax)
    movq    %rdx, 8(%rax)
    movq    %rcx, 16(%rax)
    movq    %r8, 24(%rax)
    /* mqhdr->mq_start++ */
    incq    (%rdi)
    movq    (%rdi), %rax
    cmpq    16(%rdi), %rax
    jne     3f
    movq    $1, (%rdi)
3:  ret
4:  /* block current thread (sender) */
    xorq    %rdi, %rdi
    callq sched_block
    ret
