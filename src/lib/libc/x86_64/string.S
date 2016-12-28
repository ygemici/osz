/*
 * libc/x86_64/string.S
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
 * @brief Low level string operations
 */

.global strcpy
.global strncpy
.global strcat
.global strncat
.global strcmp
.global strncmp
.global strdup
.global strndup
.global strchr
.global strrchr
.global strstr
.global strlen
.global strnlen
.global strerror
.global ffs
.global strcasecmp
.global strncasecmp
.global strsignal
.global basename
.global dirname

.section .text
strcpy:
1:  lodsb
    stosb
    or      %al, %al
    jnz     1b
    movq    %rdi, %rax
    ret

strncpy:
    movq    %rdx, %rcx
    repnz   movsb
    ret

strcat:
    ret

strncat:
    ret

strcmp:
    xorq    %rax, %rax
1:  lodsb
    cmpb    %al, (%rdi)
    jne     2f
    incq    %rdi
    or      %al, %al
    jnz     1b
2:  ret

strncmp:
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

strdup:
    ret

strndup:
    ret

strchr:
    ret

strrchr:
    ret

strstr:
    ret

strlen:
    movq    %rdi, %rsi
    xor     %rcx, %rcx
    decq    %rcx
1:  lodsb
    incq    %rcx
    or      %al, %al
    jnz     1b
    movq    %rcx, %rax
    ret

strnlen:
    ret

strerror:
    ret

ffs:
    ret

strcasecmp:
    ret

strncasecmp:
    ret

strsignal:
    ret

basename:
    ret

dirname:
    ret
