/*
 * libc/x86_64/core.S
 * 
 * Copyright 2017 CC-by-nc-sa-4.0 bztsrc@github
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
 * @brief Core functions library
 */

#define _AS 1
#include <limits.h>
#include <syscall.h>

.global dl
.global yield
.global seterr
.global sysinfo
.global swapbuf

.section .text

/* void *dl(so, sym) */
dl:
    # dynamically link a shared object
    # rdi: pointer to .so file path
    # rsi: pointer to symbol name
    movq    %rsi, %rdx
    movq    %rdi, %rsi
    xorq    %rdi, %rdi
    movb    $SYS_dl, %dil
    movl    $0x646E6573, %eax # 'send'
    syscall
    jmp     2f

/* sysinfo_t *sysinfo() */
sysinfo:
    xorq    %rdi, %rdi
    movb    $SYS_sysinfo, %dil
    movl    $0x646E6573, %eax # 'send'
    syscall
2:  call    mq_recv@PLT
/*
2:  movq    $MQ_ADDRESS, %rbx
    movq    8(%rbx), %rax
    shlq    $6, %rax
    addq    %rbx, %rax
    incq    8(%rbx)
    movq    8(%rbx), %rcx
    cmpq    16(%rbx), %rcx
    jne     1f
    movq    $1, 8(%rbx)
1:
*/
    mov     16(%rax), %rax
    ret

/* void yield() */
yield:
    xorq    %rdi, %rdi
    movb    $SYS_sched_yield, %dil
    movl    $0x646E6573, %eax # 'send'
    syscall
    ret

/* void seterr(uint64_t err) */
seterr:
    movq    %rdi, %rsi
    xorq    %rdi, %rdi
    movb    $SYS_seterr, %dil
    movl    $0x646E6573, %eax # 'send'
    syscall
    ret

/* void swapbuf() */
swapbuf:
    movq    %rdi, %rsi
    xorq    %rdi, %rdi
    movb    $SYS_swapbuf, %dil
    movl    $0x646E6573, %eax # 'send'
    syscall
    ret
