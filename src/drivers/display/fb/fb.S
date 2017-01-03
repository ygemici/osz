/*
 * drivers/display/fb.S
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
 * @brief Frame buffer driver
 */

#define _AS 1
#include <osZ.h>

.global _init
.global mem2vid
.global aaa
.global printf

_init:
    call    printf@PLT
    movq    aaa@GOTPCREL(%rip), %rax
    xorq    %rdi, %rdi
    movb    $SYS_sysinfo, %dil
    movl    $0x646E6573, %eax # 'send'
    syscall
    call    mq_recv@PLT
/*
    movq    $MQ_ADDRESS, %rbx
    movq    8(%rbx), %rax
    shlq    $6, %rax
    addq    %rbx, %rax
    incq    8(%rbx)
    movq    8(%rbx), %rcx
    cmpq    16(%rbx), %rcx
    jne     1f
    movq    $1, 8(%rbx)
1:  mov     8(%rax), %rax
*/
    ret

mem2vid:
    ret

.section .data
screen:
    .quad 0x123456
fbuf:
    .quad 0
aaa:
    .quad 0
    
