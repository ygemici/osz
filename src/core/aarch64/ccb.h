/*
 * core/aarch64/ccb.h
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
 */

#define OSZ_CCB_MAGIC "CPUB"
#define OSZ_CCB_MAGICH 0x42555043

/* must match with ccb struct, for asm */
#define ccb_hd_timerq 0x10

/* for priority levels see src/core/tcb.h */
#ifndef _AS
typedef struct {
    uint32_t magic;     // +00 (TSS64 offset) CPU Control Block magic 'CPUB'
    uint32_t id;        // +04 core ID
    pid_t    lastxreg;  // +08 task that last used media registers
    pid_t hd_timerq;    // +10 timer queue head (alarm)
    pid_t hd_blocked;   // blocked queue head
    pid_t hd_active[8]; // priority queues (heads of active tasks)
    pid_t cr_active[8]; // priority queues (current tasks)
} __attribute__((packed)) ccb_t;

c_assert(sizeof(ccb_t) <= __PAGESIZE);

#endif
