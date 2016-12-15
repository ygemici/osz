/*
 * core/x86_64/ccb.h
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
 * @brief CPU Control Block. Architecture specific implementation
 * MUST ALIGN WITH 64 BIT TSS
 */

#define OSZ_CCB_MAGIC "CPUB"
#define OSZ_CCB_MAGICH 0x42555043

#define EXCERR_OFFS 0x4C

// mutex flags
#define MUTEX_OFFS 0x5C
#define LOCK_TASKSWITCH 0

#ifndef _AS
typedef struct {
    uint32_t magic;     // +00 (TSS64 offset)
    uint64_t rsp0;      // +04
    uint64_t rsp1;      // +0C
    uint64_t rsp2;      // +14
    uint32_t realid;    // +1C real APIC ID
    uint32_t id;        // +20 logical APIC ID
    uint64_t ist1;      // +24 user (exception, syscall) stack
    uint64_t ist2;      // +2C NMI stack
    uint64_t ist3;      // +34 IRQ stack
    uint64_t ist4;      // +3C not used at the moment
    uint64_t ist5;      // +44 not used at the moment
    uint64_t excerr;    // +4C exception error code
    OSZ_tcb *lastxreg;  // +54 thread that last used extra registers
    uint32_t mutex[3];  // +5C
    uint16_t iomapbase; // +66 IO permission map base, not used
    uint16_t dummy0;
    OSZ_tcb *hd_timerq; // +6A timer queue head (alarm)
    OSZ_tcb *hd_blocked;// blocked queue head
    OSZ_tcb *hd_active[8];  // priority queues (heads of active threads)
    OSZ_tcb *cr_active[8];  // priority queues (current threads)
} __attribute__((packed)) OSZ_ccb;
#endif
