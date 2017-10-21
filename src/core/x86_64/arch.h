/*
 * core/x86_64/arch.h
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
#define systable_smbi_idx 1
#define systable_efi_idx 2
#define systable_mp_idx 3
#define systable_dsdt_idx 4
#define systable_apic_idx 5
#define systable_ioapic_idx 6
#define systable_hpet_idx 7

#define task_map(m) __asm__ __volatile__ ("mov %0, %%rax; mov %%rax, %%cr3" : : "a"(m));
#define breakpoint __asm__ __volatile__("xchg %bx, %bx")

/* VMM access bits */
#define PG_CORE             0b00011
#define PG_CORE_RO          0b00001
#define PG_CORE_NOCACHE     0b11011
#define PG_USER_RO          0b00101
#define PG_USER_RW          0b00111
#define PG_USER_RWNOCACHE   0b10111
#define PG_USER_DRVMEM      0b11111
#define PG_PAGE          0b00000001
#define PG_SLOT          0b10000000
#define PG_NX_BIT           63
