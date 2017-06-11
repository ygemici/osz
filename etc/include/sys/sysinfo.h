/*
 * sys/sysinfo.h
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
 * @brief OS/Z System Information Structure
 */

#ifndef	_SYS_SYSINFO_H
#define	_SYS_SYSINFO_H	1

#include <lastbuild.h>

#define SYSINFO_MAGIC "SINF"
#define SYSINFO_MAGICH 0x464e4953

#ifndef _AS
typedef struct {
    uint32_t magic;         // magic
    uint16_t fps;           // last sec frame per second
    uint8_t reserved1;
    uint8_t reserved2;
    uint64_t mem_free;      // number of free pages
    uint64_t mem_total;     // total number of pages
    uint64_t ticks[4];      // overall counters
    uint64_t srand[4];      // random seed bits
    uint64_t systables[16]; // platform specific addresses
    char osver[128];        // OS version in human readable form
} __attribute__((packed)) sysinfo_t;

sysinfo_t *sysinfo();                   // query system information
#endif

/* ticks indeces for counters */
#define TICKS_TS 0      //+00 timestamp sec counter
#define TICKS_NTS 1     //+08 timestamp nanosec fraction
#define TICKS_LO 2      //+16 overall ticks (jiffies, 128 bit)
#define TICKS_HI 3      //+24

/*** Platform independent ***/
#define sysinfo_magic 0
#define sysinfo_fps 4
#define sysinfo_mem_free 8
#define sysinfo_mem_total 16
#define sysinfo_ticks 24
#define sysinfo_ts 24
#define sysinfo_nts 32
#define sysinfo_jiffy_lo 40
#define sysinfo_jiffy_hi 48
#define sysinfo_srand0 56
#define sysinfo_srand1 64
#define sysinfo_srand2 72
#define sysinfo_srand3 80

/*** Platform specific ***/
#define sysinfo_systables 88

#ifdef OSZ_ARCH_Aarch64
#endif

#ifdef OSZ_ARCH_x86_64
#define sysinfo_acpi_ptr 88
#define sysinfo_smbi_ptr 96
#define sysinfo_efi_ptr 104
#define sysinfo_mp_ptr 112
#define sysinfo_dsdt_ptr 120
#define sysinfo_apic_ptr 128

#define systable_acpi_idx 0
#define systable_smbi_idx 1
#define systable_efi_idx 2
#define systable_mp_idx 3
#define systable_dsdt_idx 4
#define systable_apic_idx 5
#endif

#endif /* sys/sysinfo.h */
