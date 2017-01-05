/*
 * core/x86_64/platform.h
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
 * @brief Platform dependent headers
 */

#include <errno.h>
#include <limits.h>
#ifndef _AS
#include <stdint.h>
#include <sys/types.h>
typedef uint64_t phy_t;
typedef uint64_t virt_t;
#endif
#include <syscall.h>
#include "tcb.h"
#include "ccb.h"
#include "isr.h"
#include "../core.h"

#define thread_map(m) __asm__ __volatile__ ("mov %0, %%rax; mov %%rax, %%cr3" : : "r"(m) : "%rax");
#define breakpoint __asm__ __volatile__("xchg %%bx, %%bx":::)

/* VMM access bits */
#define PG_CORE 0b00011
#define PG_CORE_NOCACHE 0b11011
#define PG_USER_RO 0b00101
#define PG_USER_RW 0b00111
#define PG_USER_RWNOCACHE 0b10111
#define PG_USER_DRVMEM 0b01111
#define PG_SLOT 0b10000000
