/*
 * core/aarch64/arch.h
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
 * @brief Architecture dependent libk headers (for the core)
 */

#include <errno.h>
#include <limits.h>
#ifndef _AS
#include <stdint.h>
#include <sys/types.h>
#endif
#include <syscall.h>
#include "tcb.h"
#include "ccb.h"
#include "isr.h"
#include "../core.h"

#define task_map(m)
#define breakpoint
#define dataseg __attribute__ ((section (".data")))

/* we don't have libc's stdarg */
#ifndef _AS
typedef unsigned char *valist;
#define vastart(list, param) (list = (((valist)&param) + sizeof(void*)*6))
// does anybody know why do we need *8 here? Empirically correct
#define vastart_sprintf(list, param) (list = (((valist)&param) + sizeof(void*)*8))
#define vaarg(list, type)    (*(type *)((list += sizeof(void*)) - sizeof(void*)))
#endif

/* VMM access bits */
#define PG_CORE 0b00011
#define PG_CORE_NOCACHE 0b11011
#define PG_USER_RO 0b00101
#define PG_USER_RW 0b00111
#define PG_USER_RWNOCACHE 0b10111
#define PG_USER_DRVMEM 0b11111
#define PG_SLOT 0b10000000

