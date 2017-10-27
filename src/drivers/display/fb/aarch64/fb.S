/*
 * drivers/display/fb_aarch64/fb.S
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
#include <sys/driver.h>

/* linker stuff */
.type _init, STT_FUNC
.global _init
.type task_init, STT_FUNC
.global task_init
.type _environment, STT_OBJECT
.size _environment, 8
.global _environment
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

.section .text

task_init:
    ret

/* main loop */
_init:

.getwork:
    b       .getwork

.section .data
/* that stupid GNU as and ld can't refer to a label rip relatively
   when it's scope is global... so we have to double all the labels. */
_environment:
environment:
    .quad 0
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
    .quad 0
    .quad 0
_cursor_ptr:
cursor_ptr:
    .quad _binary_shapes_start
cursor_x:
    .long 0 /* right hand */
    .long 0 /* left hand */
cursor_y:
    .long 0
    .long 0
