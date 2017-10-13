/*
 * loader/aarch64-rpi/boot.S
 * 
 * Copyright 2017 Public Domain BOOTBOOT bztsrc@github
 *
 * This file is part of the BOOTBOOT Protocol package.
 * @brief Boot loader for the Raspberry Pi 3+ ARMv8
 *
 */
.section ".text.boot"

.global _start
.global jumptokernel

/*********************************************************************
 *                Entry point called by start.elf                    *
 *********************************************************************/
_start:
    // magic
    b       1f
    .ascii  "BOOTBOOT"
    // read cpu id, stop slave cores
1:  mrs     x7, mpidr_el1
    and     x7, x7, #3
    cbz     x7, 2f
1:  wfe
    b       1b
2:  // set stack before our code
    mov     sp, #0x80000
    // clear bss
    ldr     w7, =__bss_start
    ldr     w8, =__bss_size
1:  cbz     x8, 2f
    str     xzr, [x7], #8
    sub     x8, x8, #1
    cbnz    x8, 1b
    // jump to C code
2:  bl      bootboot_main
1:  wfe
    b       1b

jumptokernel:
    mov     w30, w0
    ret
