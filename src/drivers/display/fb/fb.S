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
#include <sys/video.h>

/* linker stuff */
.type _init, STT_FUNC
.global _init
.type _screen_ptr, STT_OBJECT
.size _screen_ptr, 8
.global _screen_ptr
.type _fb_ptr, STT_OBJECT
.size _fb_ptr, 8
.global _fb_ptr
.type _fb_width, STT_OBJECT
.size _fb_width, 4
.global _fb_width
.type _fb_height, STT_OBJECT
.size _fb_height, 4
.global _fb_height
.type _fb_scanline, STT_OBJECT
.size _fb_scanline, 4
.global _fb_scanline
.type _fb_type, STT_OBJECT
.size _fb_type, 1
.global _fb_type
.type _display_type, STT_OBJECT
.size _display_type, 1
.global _display_type
.global _cursor_w
.global _cursor_h
.global _cursor_ptr

.extern loadcursor
.extern _binary_shapes_start
.extern _binary_shapes_end

.section .text

/* main loop */
_init:

.getwork:
    call    mq_recv@PLT
    cmpw    $VID_flush, (%rax)
    jne     1f
    /* copy screen to framebuffer */
    movq    screen_ptr(%rip), %rsi
    movq    fb_ptr(%rip), %rbx
    xorq    %rcx, %rcx
    movl    fb_width(%rip), %edx
    shrl    $1, %edx
    xorq    %r8, %r8
    movl    fb_height(%rip), %r8d
    xorq    %r9, %r9
    movl    fb_scanline(%rip), %r9d
2:  movq    %rbx, %rdi
    movl    %edx, %ecx
    repnz   movsq
    addq    %r9, %rbx
    dec     %r8d
    jnz     2b
    jmp     .getwork

1:  cmpw    $VID_showcursor, (%rax)
    jne     1f
    jmp     .getwork

1:  cmpw    $VID_loadcursor, (%rax)
    jne     1f
    movq    8(%rax), %rdi
    callq   loadcursor
    jmp     .getwork

1:  cmpw    $VID_setcursor, (%rax)
    jne     .getwork
    jmp     .getwork

.section .data
/* that stupid GNU as and ld can't refer to a label rip relatively
   when it's scope is global... so we have to double all labels. */
_screen_ptr:
screen_ptr:
    .quad 0
_fb_ptr:
fb_ptr:
    .quad 0
_fb_width:
fb_width:
    .long 0
_fb_height:
fb_height:
    .long 0
_fb_scanline:
fb_scanline:
    .long 0
_fb_type:
fb_type:
    .byte 0
_display_type:
display_type:
    .byte 0
_cursor_w:
cursor_w:
    .word 32
_cursor_h:
cursor_h:
    .word 992
cursor_shape:
    .long 0
_cursor_ptr:
cursor_ptr:
    .quad _binary_shapes_start
