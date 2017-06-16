/*
 * lib/libc/x86_64/string.S
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
 * @brief Low level string operations.
 */

/* All functions must be direction flag independent, and must behave well on NULL input.
 * What's more str* functions must be UTF-8 aware. */

.global memcpy
.global memzero
.global memmove
.global memset
.global memcmp
.global memchr
.global ffs
.global strcpy
.global strncpy
.global strcat
.global strncat
.global strcmp
.global strncmp
.global strchr
.global strrchr
.global strlen
.global strnlen
.global mbstrlen
.global mbstrnlen
.global strcasecmp
.global strncasecmp

.section .text

memzero:
    cld
    orq     %rdi, %rdi
    jz      2f
    orq     %rsi, %rsi
    jz      2f
    movq    %rdi, %rbx
    movq    %rsi, %rcx
    xorq    %rax, %rax
    shrq    $3, %rcx
    or      %rcx, %rcx
    jz      1f
    repnz   stosq
1:  movb    %sil, %cl
    andb    $0x7, %cl
    repnz   stosb
    movq    %rbx, %rax
2:  ret

memmove:
    orq     %rdi, %rdi
    jz      2f
    orq     %rsi, %rsi
    jz      2f
    orq     %rdx, %rdx
    jz      2f
    movq    %rsi, %rax
    addq    %rdx, %rax
    cmpq    %rdi, %rax
    jb      1f
    addq    %rdx, %rdi
    addq    %rdx, %rsi
    std
    jmp     1f
memcpy:
    cld
    orq     %rdi, %rdi
    jz      2f
    orq     %rsi, %rsi
    jz      2f
    orq     %rdx, %rdx
    jz      2f
1:  movq    %rdi, %rbx
    movq    %rdx, %rcx
    shrq    $3, %rcx
    or      %rcx, %rcx
    jz      1f
    repnz   movsq
1:  movb    %dl, %cl
    andb    $0x7, %cl
    repnz   movsb
    cld
    movq    %rbx, %rax
2:  ret

memset:
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

memcmp:
    cld
    xorq    %rax, %rax
    orq     %rdx, %rdx
    jz      1f
    cmpq    %rsi, %rdi
    je      1f
    incb    %al
    orq     %rdi, %rdi
    jz      1f
    orq     %rsi, %rsi
    jz      1f
    movq    %rdx, %rcx
    rep     cmpsb
    jz      1f
    incb    %al
1:  ret

memchr:
    movq    %rdi, %rax
    orq     %rax, %rax
    jz      2f
1:  cmpb    (%rax), %sil
    je      2f
    incq    %rax
    decq    %rdx
    jnz     1b
    xorq    %rax, %rax
2:  ret

memrchr:
    orq     %rdi, %rdi
    jz      2f
    orq     %rdx, %rdx
    jz      2f
    decq    %rdx
    movq    %rdi, %rax
    addq    %rdx, %rax
1:  cmpb    (%rax), %sil
    je      1f
    decq    %rax
    decq    %rdx
    jnz     1b
2:  xorq    %rax, %rax
1:  ret

ffs:
    xorq    %rax, %rax
    orq     %rdi, %rdi
    jz      1f
    bsfq    (%rdi), %rax
    jz      1f
    incq    %rax
1:  ret

strcat:
1:  cmpb    $0, (%rdi)
    jz      1f
    incq    %rdi
    jmp     1b
strcpy:
1:  cld
    movq    %rdi, %rbx
    orq     %rdi, %rdi
    jz      2f
    orq     %rsi, %rsi
    jz      2f
1:  lodsb
    stosb
    or      %al, %al
    jnz     1b
2:  movq    %rbx, %rax
    ret

strncat:
1:  cmpb    $0, (%rdi)
    jz      1f
    incq    %rdi
    jmp     1b
strncpy:
1:  cld
    movq    %rdi, %rbx
    orq     %rdx, %rdx
    jz      2f
    orq     %rdi, %rdi
    jz      2f
    orq     %rsi, %rsi
    jz      2f
1:  lodsb
    stosb
    decq    %rdx
    jz      2f
    or      %al, %al
    jnz     1b
2:  movq    %rbx, %rax
    ret

strcmp:
    xorq    %rax, %rax
    cmpq    %rsi, %rdi
    je      2f
    incb    %al
    orq     %rdi, %rdi
    jz      2f
    orq     %rsi, %rsi
    jz      2f
1:  lodsb
    cmpb    %al, (%rdi)
    jne     2f
    incq    %rdi
    or      %al, %al
    jnz     1b
2:  ret

strncmp:
    cld
    xorq    %rax, %rax
    orq     %rdx, %rdx
    jz      3f
    cmpq    %rsi, %rdi
    je      3f
    incb    %al
    orq     %rdi, %rdi
    jz      3f
    orq     %rsi, %rsi
    jz      3f
    incq    %rdx
1:  decq    %rdx
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

strlen:
    cld
    movq    %rdi, %rsi
    xor     %rcx, %rcx
    orq     %rsi, %rsi
    jz      2f
    decq    %rcx
1:  lodsb
    incq    %rcx
    or      %al, %al
    jnz     1b
2:  movq    %rcx, %rax
    ret

strnlen:
    cld
    movq    %rsi, %rdx
    movq    %rdi, %rsi
    xor     %rcx, %rcx
    orq     %rsi, %rsi
    jz      2f
    orq     %rdx, %rdx
    jz      2f
    decq    %rcx
1:  lodsb
    incq    %rcx
    decq    %rdx
    jz      2f
    or      %al, %al
    jnz     1b
2:  movq    %rcx, %rax
    ret

mbstrlen:
    cld
    movq    %rdi, %rsi
    xor     %rcx, %rcx
    orq     %rsi, %rsi
    jz      2f
    decq    %rcx
1:  lodsb
    incq    %rcx
    btw     $7, %ax
    jnc     3f
    incq    %rsi
    movb    %al, %ah
    shrb    $5, %al
    cmpb    $0b110, %al
    je      3f
    incq    %rsi
    movb    %ah, %al
    shrb    $4, %al
    cmpb    $0b1110, %al
    je      3f
    incq    %rsi
3:  or      %al, %al
    jnz     1b
2:  movq    %rcx, %rax
    ret

mbstrnlen:
    cld
    movq    %rsi, %rdx
    movq    %rdi, %rsi
    xor     %rcx, %rcx
    orq     %rsi, %rsi
    jz      2f
    decq    %rcx
1:  lodsb
    incq    %rcx
    btw     $7, %ax
    jnc     3f
    incq    %rsi
    movb    %al, %ah
    shrb    $5, %al
    cmpb    $0b110, %al
    je      3f
    incq    %rsi
    movb    %ah, %al
    shrb    $4, %al
    cmpb    $0b1110, %al
    je      3f
    incq    %rsi
3:  decq    %rdx
    jz      2f
    or      %al, %al
    jnz     1b
2:  movq    %rcx, %rax
    ret

/* NOTE: utf-8 multibyte safe */
strchr:
    movq    %rdi, %rax
    orq     %rax, %rax
    jz      5f
    cmpl    $256, %esi
    jb      1f
    cmpl    $65536, %esi
    jb      2f
    movl    %esi, %ebx
    shrl    $16, %ebx
    cmpl    $65536*256, %esi
    jb      3f
4:  cmpw    (%rax), %si
    je      5f
    incq    %rax
    decq    %rdx
    jnz     4b
    xorq    %rax, %rax
5:  ret
1:  cmpb    (%rax), %sil
    je      1f
    incq    %rax
    decq    %rdx
    jnz     1b
    xorq    %rax, %rax
1:  ret
2:  cmpw    (%rax), %si
    je      2f
    incq    %rax
    decq    %rdx
    jnz     2b
    xorq    %rax, %rax
2:  ret
3:  cmpw    (%rax), %si
    jne     2f
    cmpb    2(%rax), %bl
    je      3f
2:  incq    %rax
    decq    %rdx
    jnz     3b
    xorq    %rax, %rax
3:  ret

/* NOTE: utf-8 multibyte safe */
strrchr:
    orq     %rdi, %rdi
    jz      5f
    cmpb    $0, (%rdi)
    jz      5f
    movq    %rdi, %rax
1:  incq    %rax
    cmpb    $0, (%rax)
    jnz     1b
    decq    %rax
    cmpl    $256, %esi
    jb      1f
    cmpl    $65536, %esi
    jb      2f
    movl    %esi, %ebx
    shrl    $16, %ebx
    cmpl    $65536*256, %esi
    jb      3f
4:  cmpl    (%rax), %esi
    je      4f
    decq    %rax
    decq    %rdx
    jnz     4b
5:  xorq    %rax, %rax
4:  ret
1:  cmpb    (%rax), %sil
    je      1f
    decq    %rax
    decq    %rdx
    jnz     1b
    xorq    %rax, %rax
1:  ret
2:  cmpw    (%rax), %si
    je      2f
    decq    %rax
    decq    %rdx
    jnz     2b
    xorq    %rax, %rax
2:  ret
3:  cmpw    (%rax), %si
    jne     2f
    cmpb    2(%rax), %bl
    je      3f
2:  decq    %rax
    decq    %rdx
    jnz     3b
    xorq    %rax, %rax
3:  ret

strcasecmp:
    ret

strncasecmp:
    ret
