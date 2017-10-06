/*
 * sys/core.h
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
 * @brief OS/Z Core services (syscall numbers). Include with osZ.h
 */

#ifndef _SYS_CORE_H
#define _SYS_CORE_H 1

#define SYS_IRQ          0 // CORE sends it to device drivers, disables IRQ
#define SYS_ack          1 // device drivers to CORE, re-enable IRQ, libc return value
#define SYS_nack         2 // negative acknowledge, libc return errno
#define SYS_exit         3
#define SYS_sched_yield  4
#define SYS_setirq       5
#define SYS_dl           6
#define SYS_swapbuf      7
#define SYS_stimebcd     8 // driver to CORE, cmos local date
#define SYS_stime        9
#define SYS_time        10
#define SYS_alarm       11
#define SYS_meminfo     12
#define SYS_mmap        14 // mman functions
#define SYS_munmap      15
#define SYS_msync       16
#define SYS_mlock       17
#define SYS_munlock     18
#define SYS_p2pcpy      19
#define SYS_rand        20
#define SYS_srand       21
#define SYS_fork        22 // process functions
#define SYS_exec        23

#define SYS_recv    0x7FFF // receive message

#endif /* sys/core.h */
