/*
 * drivers/display/fb/fb.S
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
.global task_init
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

task_init:
    ret

/* main loop */
_init:

.getwork:
    call    mq_recv@PLT

    cmpw    $VID_flush, (%rax)
    jne     1f
.flush:
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
    callq   showcursor
    jmp     .getwork

1:  cmpw    $VID_movecursor, (%rax)
    jne     1f
.movecursor:
    pushq   %rax
    callq   hidecursor
    popq    %rax
    movl    16(%rax), %ecx
    movl    24(%rax), %edx
    cmpb    $0, 8(%rax)
    jnz     2f
    movl    %ecx, cursor_x(%rip)
    movl    %edx, cursor_y(%rip)
    jmp     3f
2:  movl    %ecx, cursor_x+4(%rip)
    movl    %edx, cursor_y+4(%rip)
3:  callq   showcursor
    jmp     .getwork

1:  cmpw    $VID_loadcursor, (%rax)
    jne     1f
    movq    8(%rax), %rdi
    callq   loadcursor
    jmp     .getwork

1:  cmpw    $VID_setcursor, (%rax)
    jne     .getwork
.setcursor:
    pushq   %rax
    callq   hidecursor
    popq    %rax
    xorq    %rbx, %rbx
    movb    8(%rax), %bl
    movq    16(%rax), %rax
    xorq    %rdx, %rdx
    xorq    %rcx, %rcx
    movw    cursor_w(%rip), %cx
    mull    %ecx
    cmpw    cursor_h(%rip), %ax
    jb      2f
    xorq    %rax, %rax
2:  mull    %ecx
    shll    $2, %eax
    orb     %bl, %bl
    jnz     2f
    movl    %eax, cursor_shape(%rip)
    callq   showcursor
    jmp     .getwork
2:  movl    %eax, cursor_shape+8(%rip)
    callq   showcursor
    jmp     .getwork

hidecursor:
    /* copy cursors' area from screen to fb */
    xorq    %r8, %r8
    xorq    %r9, %r9
    xorq    %r10, %r10
    xorq    %r11, %r11
    movq    screen_ptr(%rip), %rsi
    movl    cursor_x(%rip), %r8d
    movl    cursor_y(%rip), %r9d
    movl    fb_width(%rip), %r10d
    xorq    %r11, %r11
    callq   copyarea
    movq    screen_ptr(%rip), %rsi
    movl    cursor_x+4(%rip), %r8d
    movl    cursor_y+4(%rip), %r9d
    movl    fb_width(%rip), %r10d
    xorq    %r11, %r11
    callq   copyarea
    ret

showcursor:
    /* copy cursors to fb with alpha */
    xorq    %r8, %r8
    xorq    %r9, %r9
    xorq    %r10, %r10
    xorq    %r11, %r11
    movq    cursor_ptr(%rip), %rsi
    addq    cursor_shape(%rip), %rsi
    movl    cursor_x(%rip), %r8d
    movl    cursor_y(%rip), %r9d
    movw    cursor_w(%rip), %r10w
    xorq    %r11, %r11
    incb    %r11b
    callq   copyarea
    movq    cursor_ptr(%rip), %rsi
    addq    cursor_shape+8(%rip), %rsi
    movl    cursor_x+4(%rip), %r8d
    movl    cursor_y+4(%rip), %r9d
    movw    cursor_w(%rip), %r10w
    xorq    %r11, %r11
    incb    %r11b
    incb    %r11b
    callq   copyarea
    ret

copyarea:
    /* r8-=cursor_w/2; r9-=cursor_w/2; clip to fb */
    xorq    %r12, %r12
    xorq    %r13, %r13
    movw    cursor_w(%rip), %r12w
    movw    %r12w, %r13w
    xorq    %rbx, %rbx
    movw    cursor_w(%rip), %bx
    shrq    $1, %rbx
    subq    %rbx, %r9
    jns     1f
    addq    %r9, %r12
    neg     %r9
    xorq    %rdx, %rdx
    movq    %r10, %rax
    shlq    $2, %r9
    mulq    %r9
    addq    %rax, %rsi
    xorq    %r9, %r9
1:  movq    %r10, %r15
    shlq    $2, %r15
    subq    %rbx, %r8
    jns     1f
    addq    %r8, %r13
    cmpb    $2, %r11b
    je      2f
    neg     %r8
    shlq    $2, %r8
    addq    %r8, %rsi
2:  xorq    %r8, %r8
    movq    %r13, %r15
    shlq    $2, %r15
1:  movl    fb_width(%rip), %eax
    decl    %eax
    subl    %r8d, %eax
    cmpl    %eax, %r13d
    jb     1f
    movl    %eax, %r13d
1:  movl    fb_height(%rip), %eax
    decl    %eax
    subl    %r9d, %eax
    cmpl    %eax, %r12d
    jb     1f
    movl    %eax, %r12d
1:
    movl    %r9d, %eax
    xorq    %rbx, %rbx
    movl    fb_scanline(%rip), %ebx
    movq    %rbx, %r14
    xorq    %rdx, %rdx
    mulq    %rbx
    shlq    $2, %r8
    shlq    $2, %r10
    addq    %r8, %rax
    addq    fb_ptr(%rip), %rax
    /* rax= ptr + y*scanline+x*4 */
    movq    %rax, %rbx
    movq    %rsi, %rdx
1:  movq    %rbx, %rdi
    movq    %rdx, %rsi
    movw    %r13w, %cx
    orb     %r11b, %r11b
    jnz     8f
    repnz   movsd
    jmp     9f
8:  cmpb    $1, %r11b
    jne     8f
2:  lodsl
    cmpl    $0x1000000, %eax
    jb      3f
    movl    %eax, (%rdi)
3:  addq    $4, %rdi
    decw    %cx
    jnz     2b
    jmp     9f
8:  addq    %r15, %rsi
2:  subq    $4, %rsi
    movl    (%rsi), %eax
    cmpl    $0x1000000, %eax
    jb      3f
    movl    %eax, (%rdi)
3:  addq    $4, %rdi
    decw    %cx
    jnz     2b
    jmp     9f
9:  addq    %r14, %rbx
    addq    %r10, %rdx
    decw    %r12w
    jnz     1b
    ret

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
