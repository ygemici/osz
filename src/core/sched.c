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

extern OSZ_pmm pmm;
extern OSZ_ccb ccb;
extern uint64_t isr_ticks[2];

OSZ_tcb *sched_get_tcb(pid_t thread)
{
    // last mapped or last used tcb
    if((uint64_t)thread==(uint64_t)(&tmpmap) ||
       (uint64_t)thread==(uint64_t)pmm.bss_end)
        return (OSZ_tcb*)(thread);
    // a new tcb
    if(thread!=0) {
        // map tcb
        kmap((uint64_t)&tmpmap, (uint64_t)(thread * __PAGESIZE), PG_CORE_NOCACHE);
        return (OSZ_tcb*)(&tmpmap);
    }
    // active tcb
    return (OSZ_tcb*)0;
}

// block until alarm
void sched_alarm(pid_t thread, uint64_t at)
{
    OSZ_tcb *tcb = sched_get_tcb(thread);
    /* TODO: ccb.hd_active -> ccb.hd_alarm */
    tcb->state = tcb_state_hybernated;
}

// hybernate a thread
void sched_sleep(pid_t thread)
{
    OSZ_tcb *tcb = sched_get_tcb(thread);
    /* TODO: ccb.hd_blocked -> swap */
    tcb->state = tcb_state_hybernated;
}

// awake a hybernated thread
void sched_awake(pid_t thread)
{
    OSZ_tcb *tcb = sched_get_tcb(thread);
    /* TODO: swap -> ccb.hd_active */
    tcb->state = tcb_state_running;
}

// add a thread to priority queue
void sched_add(pid_t thread)
{
    OSZ_tcb *tcb = sched_get_tcb(thread);
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

// move a TCB from priority queue to blocked queue
void sched_block(pid_t thread)
{
    OSZ_tcb *tcb = sched_get_tcb(thread);
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

// move a TCB from blocked queue to priority queue
void sched_activate(pid_t thread)
{
    OSZ_tcb *tcb = sched_get_tcb(thread);
    tcb->blkcnt += tcb->blktime - isr_ticks[0];
    tcb->state = tcb_state_running;
    /* TODO: ccb.hd_blocked -> ccb.hd_active */
}

// pick a thread and return it's memroot
uint64_t sched_pick()
{
    // uint64_t ptr = thread * __PAGESIZE;
    return 0;
}
