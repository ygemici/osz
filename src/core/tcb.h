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
 * @brief Thread Control Block. Common defines
 */

#define OSZ_TCB_MAGIC "THRD"
#define OSZ_TCB_MAGICH 0x44524854

enum OSZ_tcb_state
{
    tcb_receiving,
    tcb_sending,
    tcb_running,
    tcb_waiting,
    tcb_hybernated
};

// index to hd_active and cr_active queues, priority levels
enum {
    PRI_SYS, // priority 0, system, non-interruptible
    PRI_RT,  // priority 1, real time tasks queue
    PRI_DRV, // priority 2, device drivers queue
    PRI_SRV, // priority 3, service queue
    PRI_APPH,// priority 4, application high priority queue
    PRI_APP, // priority 5, application normal priority queue
    PRI_APPL,// priority 6, application low priority queue
    PRI_IDLE // priority 7, idle queue (defragmenter, screensaver etc.)
};

#define OSZ_tcb_flag_dispatching    1
#define OSZ_tcb_flag_calling        2
