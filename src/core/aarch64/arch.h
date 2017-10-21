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

#include <limits.h>
#ifndef _AS
#include <stdint.h>
#include <sys/types.h>
#endif
#include "tcb.h"
/*
#include "ccb.h"
#include "isr.h"
*/
#include "../core.h"

/* system tables */
#define systable_acpi_idx 0
#define systable_mmio_idx 1

#define task_map(m)
#define breakpoint

/* VMM access bits */
#define PG_CORE             0b011100000000  // nG=0,AF=1,SH=3,AP=0,NS=0,Attr=0
#define PG_CORE_RO          0b011110000000  // nG=0,AF=1,SH=3,AP=2,NS=0,Attr=0
#define PG_CORE_NOCACHE     0b110000001000  // nG=1,AF=1,SH=0,AP=0,NS=0,Attr=2
#define PG_USER_RO          0b011111000000  // nG=0,AF=1,SH=3,AP=3,NS=0,Attr=0
#define PG_USER_RW          0b011101000000  // nG=0,AF=1,SH=3,AP=1,NS=0,Attr=0
#define PG_USER_RWNOCACHE   0b110001001000  // nG=1,AF=1,SH=0,AP=1,NS=0,Attr=2
#define PG_USER_DRVMEM      0b110001000100  // nG=1,AF=1,SH=0,AP=1,NS=0,Attr=1

#define PG_PAGE                       0b11  // 4k granule
#define PG_SLOT                       0b01  // 2M granule
#define PG_NX_BIT           54
