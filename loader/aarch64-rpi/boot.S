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
    ldr     x7, =__bss_start
    ldr     w8, =__bss_size
1:  cbz     w8, 2f
    str     xzr, [x7], #8
    sub     w8, w8, #1
    cbnz    w8, 1b
    // jump to C code
2:  bl      bootboot_main
1:  wfe
    b       1b

jumptokernel:
    // move stack to the top of memory. We waste 16 bytes here
    // to create an address in the negative range
    mov     sp, #-16
    // set PC to kernel entry point
    mov     x30, x0
    ret

.global getttbr1
getttbr1:
    mrs     x0, ttbr1_el1
    ret
.global gettcr
gettcr:
    mrs     x0, tcr_el1
    ret
.global getsctlr
getsctlr:
    mrs     x0, sctlr_el1
    ret
