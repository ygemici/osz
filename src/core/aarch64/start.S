/*
 * core/aarch64/start.S
 *
 * Copyright 2016 CC-by-nc-sa bztsrc@github
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
 * @brief Kernel entry point
 */

#include "arch.h"

.section .text
.global _start

/**********************************************************************
 *                     OS/Z Loader entry point                        *
 **********************************************************************/
_start:
    /* setup stack frame */
    mov     sp, #-16

    /* initialize console so that we can report errors and stats */
    bl      kprintf_init
#if DEBUG
    bl      platform_dbginit
    /* clear debug status */
#endif

    /* init entropy */
    bl      platform_srand

/* jump to C function main() in 64 bit code segment */
    bl      main

    /* feature not supported */
1:  ldr     x0, =nocpu
    b       kpanic

/* placed in text intentionally, it's rodata. */
nocpu:
.asciz "CPU not supported"
