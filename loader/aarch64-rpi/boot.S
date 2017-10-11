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

/*********************************************************************
 *     Entry point called by start.elf, loaded at address 0x80000    *
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
2:  // set stack page aligned before our code
    mov     x7, x30
    and     x7, x7, #0xFFFFF000
    mov     sp, x7
    // clear bss
    ldr     w0, =__bss_start
    ldr     w1, =__bss_size
1:  cbz     x1, 2f
    str     xzr, [x0], #8
    sub     x1, x1, #1
    cbnz    x1, 1b
    // jump to C code
2:  bl      bootboot_main
1:  wfe
    b       1b
