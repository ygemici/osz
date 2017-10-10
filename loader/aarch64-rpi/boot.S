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
.global delay
.type delay, STT_FUNC

/*********************************************************************
 *    Entry point called by start.elf, must be at address 0x80000    *
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
    // setup stack before our code
2:  mov     sp, #0x80000
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

delay:
    // do something, call an empty function
1:  bl      1f
    // count down
    sub     x0, x0, #1
    cbnz    x0, 1b
1:  nop
    ret
