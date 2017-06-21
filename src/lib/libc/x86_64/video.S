/*
 * lib/libc/x86_64/video.S
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
    xorq    %rax, %rax
    movb    $SYS_swapbuf, %al
    syscall
    ret

/**
 * void vid_loadcursor(char *file);
 */
vid_loadcursor:
    movq    $SRV_video, %rax
    shlq    $16, %rax
    movb    $VID_loadcursor, %al
    syscall
    ret

/**
 * void vid_movecursor(int hand,int x,int y);
 */
vid_movecursor:
    andq    $0xFF, %rdi
    movq    $SRV_video, %rax
    shlq    $16, %rax
    movb    $VID_movecursor, %al
    syscall
    ret

/**
 * void vid_setcursor(int hand,uint8_t shape);
 */
vid_setcursor:
    andq    $0xFF, %rdi
    andq    $0xFF, %rsi
    movq    $SRV_video, %rax
    shlq    $16, %rax
    movb    $VID_setcursor, %al
    syscall
    ret
