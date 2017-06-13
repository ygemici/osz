/*
 * libc/x86_64/video.S
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
 * @brief Low level video driver implementation, see etc/include/video.h
 */

#define _AS 1
#include <limits.h>
#include <syscall.h>
#include <sys/video.h>

.global vid_swapbuf
.global vid_loadcursor
.global vid_movecursor
.global vid_setcursor

.section .text

/**
 * void vid_swapbuf()
 */
vid_swapbuf:
    movq    %rdi, %rsi
    xorq    %rdi, %rdi
    movb    $SYS_swapbuf, %dil
    movl    $0x646E6573, %eax # 'send'
    syscall
    ret

/**
 * void vid_loadcursor(char *file);
 */
vid_loadcursor:
    movq    %rdi, %rsi
    movq    $SRV_video, %rdi
    shlq    $16, %rdi
    movb    $VID_loadcursor, %dil
    movl    $0x646E6573, %eax # 'send'
    syscall
    ret

/**
 * void vid_movecursor(int hand,int x,int y);
 */
vid_movecursor:
    movq    %rdx, %r10
    movq    %rsi, %rdx
    movq    %rdi, %rsi
    movq    $SRV_video, %rdi
    shlq    $16, %rdi
    movb    $VID_movecursor, %dil
    movl    $0x646E6573, %eax # 'send'
    syscall
    ret

/**
 * void vid_setcursor(int hand,uint8_t shape);
 */
vid_setcursor:
    xorq    %rdx, %rdx
    movsxb  %sil, %rdx
    movq    %rdi, %rsi
    movq    $SRV_video, %rdi
    shlq    $16, %rdi
    movb    $VID_setcursor, %dil
    movl    $0x646E6573, %eax # 'send'
    syscall
    ret
