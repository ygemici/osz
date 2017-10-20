/*
 * core/aarch64/rpi/platform.h
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
 * @brief Platform specific definitions
 */

#define MMIO_BASE    0xfffffffffa000000

#define PM_RTSC         ((volatile uint32_t*)(MMIO_BASE+0x0010001c))
#define PM_WATCHDOG     ((volatile uint32_t*)(MMIO_BASE+0x00100024))
#define PM_WDOG_MAGIC   0x5a000000
#define PM_RTSC_FULLRST 0x00000020

#define AUX_MU_LSR      ((volatile uint32_t*)(MMIO_BASE+0x00215054))
#define AUX_MU_IO       ((volatile uint32_t*)(MMIO_BASE+0x00215040))
#define UART0_DR        ((volatile uint32_t*)(MMIO_BASE+0x00201000))
