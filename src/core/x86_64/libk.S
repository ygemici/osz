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

.section .text
.global kpanic
.global kmemcpy
.global kmemset
.global kmemcmp
.global kmemvid
.global kmap
.global kmap_init
.extern kmap_tmp
.extern tmpmap
.extern tmp2map
.extern tmppte

/* kernel panic */
kpanic:
    movl    $0xFF0000, fg
    movl    $0x200000, bg
    pushq   %rdi
    movq    $kpanicprefix, %rdi
    call    kprintf
    popq    %rdi
    addq    $8, %rsp
    call    kprintf
    cli
    hlt
kpanicprefix:
.asciz "core - E - "

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

kmemvid:
    ret

kmap:
    call    kmap_getpte
    andw    $0xF000, %si
    mov     %dl, %sil
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
    add     %rax, %rbx
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
    andw    $0x0F000, %ax
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
    invlpg  (%rbx)
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
    invlpg  (%rbx)
    invlpg  tmpmap
    addq    $tmpmap, %rax
    ret
