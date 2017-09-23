/*
 * lib/libc/x86_64/stdlib.S
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
 * @brief Low level library implementation to interact with the kernel, see etc/include/syscall.h
 */

#define _AS 1
#include <limits.h>
#include <syscall.h>

.global srand
.global rand
.global mblen
.global div
.global ldiv
.global lldiv

.section .text

/**
 * initialize random with seed
 *
 * void srand(uint64_t seed)
 */
srand:
    xorq    %rax, %rax
    movb    $SYS_srand, %al
    syscall
    ret

/**
 * Fills up almost every register with random
 *
 * uint64_t rand()
 */
rand:
    xorq    %rax, %rax
    movb    $SYS_rand, %al
    syscall
    ret

/**
 * int mblen (char *s, size_t n);
 */
mblen:
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

/**
 * div_t div (int numer, int denom);
 */
div:
    xorq    %rax, %rax
    movl    %edi, %eax
    xorl    %edx, %edx
    divl    %esi
    shlq    $32, %rdx
    addq    %rdx, %rax
    ret

/**
 * ldiv_t ldiv (long int numer, long int denom);
 * lldiv_t lldiv (long long int numer, long long int denom);
 */
ldiv:
lldiv:
    movq    %rdi, %rax
    xorq    %rdx, %rdx
    divq    %rsi
    ret
