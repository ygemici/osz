/*
 * lib/libc/x86_64/mem.S
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
 * @brief Low level memory operations
 */

.global memcpy
.global memzero
.global memmove
.global memset
.global memcmp
.global memchr

.section .text

memcpy:
    repnz   movsb
    ret

memzero:
    movq    16(%rsp), %rcx
    movq    8(%rsp), %rdi
    xor     %rax, %rax
    repnz   stosb
    ret

memmove:
    movq    24(%rsp), %rcx
    movq    16(%rsp), %rsi
    movq    8(%rsp), %rdi
    repnz   movsb
    ret

memset:
    movq    %rsi, %rax
    repnz   stosb
    ret

memcmp:
    ret

memchr:
    ret
