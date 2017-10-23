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
 * @brief CPU Control Block. Mapped per core, architecture specific
 * !!!WARNING!!! MUST ALIGN WITH 64 BIT TSS
 */

#define OSZ_CCB_MAGIC "CPUB"
#define OSZ_CCB_MAGICH 0x42555043

/* must match with ccb struct, for asm */
#define ccb_ist1 0x24
#define ccb_ist2 0x2C
#define ccb_ist3 0x34
#define ccb_mutex 0x5C
#define ccb_hd_timerq 0x68

/* for priority levels see src/core/tcb.h */
#ifndef _AS
typedef struct {
    uint32_t magic;     // +00 (TSS64 offset) CPU Control Block magic 'CPUB'
    uint64_t rsp0;      // +04
    uint64_t rsp1;      // +0C
    uint64_t rsp2;      // +14
    uint32_t realid;    // +1C real APIC ID
    uint32_t id;        // +20 logical APIC ID
    uint64_t ist1;      // +24 user (exception, syscall, IRQ) stack
    uint64_t ist2;      // +2C NMI stack
    uint64_t ist3;      // +34 debug stack
    uint64_t ist4;      // +3C not used at the moment
    uint64_t ist5;      // +44 not used at the moment
    uint64_t ist6;      // +4C not used at the moment
    pid_t    lastxreg;  // +54 task that last used media registers
    uint16_t mutex[5];  // +5C
    uint16_t iomapbase; // +66 IO permission map base, not used, must be zero
    pid_t hd_timerq;    // +68 timer queue head (alarm)
    pid_t hd_blocked;   // blocked queue head
    pid_t hd_active[8]; // priority queues (heads of active tasks)
    pid_t cr_active[8]; // priority queues (current tasks)
} __attribute__((packed)) ccb_t;

c_assert(sizeof(ccb_t) <= __PAGESIZE);

#endif
