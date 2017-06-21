/*
 * core/tcb.h
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
 * @brief Task Control Block. Common defines
 */

#define OSZ_TCB_MAGIC "TASK"
#define OSZ_TCB_MAGICH 0x4B534154

#define tcb_state_hybernated 0
#define tcb_state_blocked 1
#define tcb_state_running 2

// index to hd_active and cr_active queues, priority levels
// priority 0, system, non-interruptible
#define PRI_SYS 0
// priority 1, real time tasks queue
#define PRI_RT 1
// priority 2, device drivers queue
#define PRI_DRV 2
// priority 3, service queue
#define PRI_SRV 3
// priority 4, application high priority queue
#define PRI_APPH 4
// priority 5, application normal priority queue
#define PRI_APP 5
// priority 6, application low priority queue
#define PRI_APPL 6
 // priority 7, idle queue (defragmenter, screensaver etc.)
#define PRI_IDLE 7

