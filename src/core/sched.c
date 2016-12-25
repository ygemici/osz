/*
 * core/x86_64/sched.c
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
 * @brief Thread scheduler
 */

#include "env.h"

extern OSZ_pmm pmm;             //Physical Memory Manager (get bss)
extern OSZ_ccb ccb;             //CPU Control Block (platform specific)
extern uint64_t isr_ticks[2];   //ticks counter (128 bit)
extern uint64_t sys_mapping;    //memroot of system task

OSZ_tcb *sched_get_tcb(pid_t thread)
{
    // active tcb
    if(thread==0)
        return (OSZ_tcb*)0;
    // last mapped or last used tcb
    if((uint64_t)thread==(uint64_t)(&tmpmap) ||
       (uint64_t)thread==(uint64_t)pmm.bss_end)
        return (OSZ_tcb*)(thread);
    // map a new tcb
    kmap((uint64_t)&tmpmap, (uint64_t)(thread * __PAGESIZE), PG_CORE_NOCACHE);
    return (OSZ_tcb*)(&tmpmap);
}

// block until alarm
void sched_alarm(pid_t thread, uint64_t at)
{
#if DEBUG
    if(debug==DBG_SCHED)
        kprintf("sched_alarm(%x, %d)\n", thread, at);
#endif
    OSZ_tcb *tcb = sched_get_tcb(thread);
    /* TODO: ccb.hd_active -> ccb.hd_alarm */
    tcb->state = tcb_state_hybernated;
}

// hybernate a thread
void sched_sleep(pid_t thread)
{
#if DEBUG
    if(debug==DBG_SCHED)
        kprintf("sched_sleep(%x)\n", thread);
#endif
    OSZ_tcb *tcb = sched_get_tcb(thread);
    /* TODO: ccb.hd_blocked -> swap */
    tcb->state = tcb_state_hybernated;
}

// awake a hybernated or blocked thread
void sched_awake(pid_t thread)
{
#if DEBUG
    if(debug==DBG_SCHED)
        kprintf("sched_awake(%x)\n", thread);
#endif
    OSZ_tcb *tcb = sched_get_tcb(thread);
    if(tcb->state == tcb_state_hybernated) {
        /* TODO: swap -> ccb.hd_active */
    } else
    if(tcb->state == tcb_state_blocked) {
        /* TODO: ccb.hd_blocked -> ccb.hd_active */
        tcb->blkcnt += tcb->blktime - isr_ticks[0];
    }
    tcb->state = tcb_state_running;
}

// add a thread to priority queue
void sched_add(pid_t thread)
{
    OSZ_tcb *tcb = sched_get_tcb(thread);
#if DEBUG
    if(debug==DBG_SCHED)
        kprintf("sched_add(%x) pri %d\n", thread, tcb->priority);
#endif
    pid_t pid = tcb->mypid;
    tcb->prev = 0;
    tcb->next = ccb.hd_active[tcb->priority];
    ccb.hd_active[tcb->priority] = thread;
    if(tcb->next != 0) {
        tcb = sched_get_tcb(tcb->next);
        tcb->prev = pid;
    }
}

// remove a thread from priority queue
void sched_remove(pid_t thread)
{
    OSZ_tcb *tcb = sched_get_tcb(thread);
    pid_t next = tcb->next, prev = tcb->prev;
#if DEBUG
    if(debug==DBG_SCHED)
        kprintf("sched_remove(%x) pri %d\n", thread, tcb->priority);
#endif
    if(ccb.hd_active[tcb->priority] == tcb->mypid) {
        ccb.hd_active[tcb->priority] = next;
    }
    if(prev != 0) {
        tcb = sched_get_tcb(prev);
        tcb->next = next;
    }
    if(next != 0) {
        tcb = sched_get_tcb(next);
        tcb->prev = prev;
    }
}

// move a thread from priority queue to blocked queue
void sched_block(pid_t thread)
{
#if DEBUG
    if(debug==DBG_SCHED)
        kprintf("sched_block(%x)\n", thread);
#endif
    /* never block the system task */
    if(thread == subsystems[SRV_system])
        return;
    OSZ_tcb *tcb = sched_get_tcb(thread);
    /* failsafe check */
    if(tcb->memroot == sys_mapping)
        return;
    tcb->blktime = isr_ticks[0];
    tcb->state = tcb_state_blocked;
    /* ccb.hd_active -> ccb.hd_blocked */
    sched_remove((pid_t)tcb);
    // restore mapping
    sched_get_tcb(thread);
    tcb->next = ccb.hd_blocked;
    tcb->prev = 0;
    ccb.hd_blocked = thread;
}

// pick a thread and return it's memroot
uint64_t sched_pick()
{
    // uint64_t ptr = thread * __PAGESIZE;
    return 0;
}
