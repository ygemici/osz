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

extern OSZ_ccb ccb;             //CPU Control Block (platform specific)
extern uint64_t isr_ticks[8];   //ticks counter (128 bit)

OSZ_tcb *sched_get_tcb(pid_t thread)
{
    // active tcb
    if(thread==0)
        return (OSZ_tcb*)0;
    // last mapped or last used tcb
    if((uint64_t)thread==(uint64_t)(&tmpmap) ||
       (uint64_t)thread==(uint64_t)(&tmp2map) ||
       (uint64_t)thread==(uint64_t)(&tmpalarm) ||
       (uint64_t)thread==(uint64_t)pmm.bss_end)
        return (OSZ_tcb*)(thread);
    // map a new tcb
    kmap((uint64_t)&tmpmap, (uint64_t)(thread * __PAGESIZE), PG_CORE_NOCACHE);
    return (OSZ_tcb*)(&tmpmap);
}

#define sched_yield() mq_recv()

// block until alarm, add thread to ccb.hd_timerq list
// note that isr_alarm() consumes threads from timer queue
void sched_alarm(OSZ_tcb *tcb, uint64_t sec, uint64_t nsec)
{
#if DEBUG
    if(debug&DBG_SCHED)
        kprintf("sched_alarm(%x, %d.%d)\n", tcb->mypid, sec, nsec);
#endif
    pid_t pid = tcb->mypid, next=ccb.hd_timerq, prev=0;
    /* ccb.hd_active -> ccb.hd_timerq */
    sched_block(tcb);
    // restore mapping
    tcb = sched_get_tcb(pid);
    tcb->alarmsec = sec;
    tcb->alarmns = nsec;
    tcb->state |= tcb_flag_alarm;
    do {
        OSZ_tcb *t = sched_get_tcb(next);
        pid_t tmp = next ? t->alarm : 0;
        if(next==0 || (t->alarmsec >= sec &&
           t->alarmns  >= nsec)) {
            if(prev!=0) {
                OSZ_tcb *t = sched_get_tcb(prev);
                t->alarm = pid;
                tcb = sched_get_tcb(pid);
            } else {
                ccb.hd_timerq = pid;
            }
            tcb->alarm = next;
            break;
        }
        prev = next;
        next = tmp;
    } while(next!=0);
    // map the tcb at the head of the queue for easy access
    kmap((uint64_t)&tmpalarm, (uint64_t)(ccb.hd_timerq * __PAGESIZE), PG_CORE_NOCACHE);
}

// hybernate a thread
void sched_sleep(OSZ_tcb *tcb)
{
#if DEBUG
    if(debug&DBG_SCHED)
        kprintf("sched_sleep(%x)\n", tcb->mypid);
#endif
    /* TODO: ccb.hd_blocked -> swap */
    tcb->state = tcb_state_hybernated;
}

// awake a hybernated or blocked thread
void sched_awake(OSZ_tcb *tcb)
{
#if DEBUG
    if(debug&DBG_SCHED)
        kprintf("sched_awake(%x)\n", tcb->mypid);
#endif
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
void sched_add(OSZ_tcb *tcb)
{
    pid_t pid = tcb->mypid;
#if DEBUG
    if(debug&DBG_SCHED)
        kprintf("sched_add(%x) pri %d\n", pid, tcb->priority);
#endif
    tcb->prev = 0;
    tcb->next = ccb.hd_active[tcb->priority];
    ccb.hd_active[tcb->priority] = pid;
    if(tcb->next != 0) {
        tcb = sched_get_tcb(tcb->next);
        tcb->prev = pid;
    }
}

// remove a thread from priority queue
void sched_remove(OSZ_tcb *tcb)
{
    pid_t next = tcb->next, prev = tcb->prev;
#if DEBUG
    if(debug&DBG_SCHED)
        kprintf("sched_remove(%x) pri %d\n", tcb->mypid, tcb->priority);
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

// move a TCB from priority queue to blocked queue
void sched_block(OSZ_tcb *tcb)
{
#if DEBUG
    if(debug&DBG_SCHED)
        kprintf("sched_block(%x)\n", tcb);
#endif
    /* never block the system task */
    if(tcb->mypid == services[-SRV_SYS] ||
       tcb->memroot == sys_mapping)
        return;
    tcb->blktime = isr_ticks[0];
    tcb->state = tcb_state_blocked;
    /* ccb.hd_active -> ccb.hd_blocked */
    pid_t pid = tcb->mypid;
    sched_remove(tcb);
    // restore mapping
    tcb = sched_get_tcb(pid);
    // link as the first item in chain
    tcb->next = ccb.hd_blocked;
    tcb->prev = 0;
    ccb.hd_blocked = pid;
}

// pick a thread and return it's memroot
uint64_t sched_pick()
{
    // uint64_t ptr = thread * __PAGESIZE;
    return 0;
}
