/*
 * loader/aarch64-rpi/boot.S
 * 
 * Copyright 2017 Public Domain bztsrc@github
 *
 * This file is part of the BOOTBOOT Protocol package.
 * @brief Boot loader for the Raspberry Pi 3 Armv8
 */

.section ".text.boot"

.global _start

_start:
/*
    // Setup the stack.
    mov     sp, #0x8000
 
    // Clear out bss.
    ldr     r4, =__bss_start
    ldr     r9, =__bss_end
    mov     r5, #0
    mov     r6, #0
    mov     r7, #0
    mov     r8, #0
    b       2f

    // store multiple at r4.
1:  stmia   r4!, {r5-r8}
 
    // If we are still below bss_end, loop.
2:  cmp     r4, r9
    blo     1b
 
    ldr     r3, =bootboot_main
    blx     r3
 
halt:
    wfe
    b       halt
*/
.align 16
//magic
.ascii "BOOTBOOT"
