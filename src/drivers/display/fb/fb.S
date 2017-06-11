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

.type _init, STT_FUNC
.global _init
.type mem2vid, STT_FUNC
.global mem2vid
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

_init:
    call    sysinfo@PLT

.getwork:
    call    mq_recv@PLT
    jmp     .getwork
    ret

mem2vid:
    ret

.section .data
_screen_ptr:
    .quad 0x123456
_fb_ptr:
    .quad 0
_fb_width:
    .long 0
_fb_height:
    .long 0
_fb_scanline:
    .long 0
_fb_type:
    .byte 0
_display_type:
    .byte 0
