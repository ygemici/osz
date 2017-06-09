/*
 * core/x86_64/sched.c
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
 * @brief Thread scheduler
 */

#include "env.h"

extern OSZ_ccb ccb;             //CPU Control Block (platform specific)
extern sysinfo_t sysinfostruc;  //sysinfo structure
extern uint64_t isr_next;       //next thread to map when isr finishes
extern uint8_t idle_first;      //flag to indicate first idle schedule

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

#if DEBUG
void sched_dump()
{
    OSZ_tcb *tcb, *curr = (OSZ_tcb*)0;
    int i;
    pid_t p;
    for(i=PRI_SYS; i<=PRI_IDLE; i++) {
        kprintf("active %d hd %x:   ",i,ccb.hd_active[i]);
        p=ccb.hd_active[i];
        while(p!=0) {
            tcb=sched_get_tcb(p);
            kprintf("%s%x%s(%x,%x) ",
                tcb->mypid==ccb.cr_active[i]?">":"",
                tcb->mypid,
                tcb->mypid==curr->mypid?"*":"",
                tcb->prev,tcb->next);
            p=tcb->next;
        }
        kprintf("\n");
    }
    kprintf("blocked hd %x:   ",ccb.hd_blocked);
    p=ccb.hd_blocked;
    while(p!=0) {
        tcb=sched_get_tcb(p);
        kprintf("%x%s(%x,%x) ",
            tcb->mypid,
            tcb->mypid==curr->mypid?"*":"",
            tcb->prev,tcb->next);
        p=tcb->next;
    }
    kprintf("\n");
    kprintf("timerq hd %x:   ",ccb.hd_timerq);
    p=ccb.hd_timerq;
    while(p!=0) {
        tcb=sched_get_tcb(p);
        kprintf("%x%s(%x) ",
            tcb->mypid,
            tcb->mypid==curr->mypid?"*":"",
            tcb->alarm);
        p=tcb->alarm;
    }
    kprintf("\n\n");
}
#endif

// block until alarm, add thread to ccb.hd_timerq list too
// note that isr_timer() consumes threads from timer queue
void sched_alarm(OSZ_tcb *tcb, uint64_t sec, uint64_t nsec)
{
    OSZ_tcb *tcba = (OSZ_tcb*)&tmpalarm;
    /* if alarm time is in the past, make sure it's running and return */
    if(sysinfostruc.ticks[TICKS_TS]>sec || 
        (sysinfostruc.ticks[TICKS_TS]==sec && sysinfostruc.ticks[TICKS_NTS]>nsec)) {
            sched_awake(tcb);
            return;
    }
#if DEBUG
    if(sysinfostruc.debug&DBG_SCHED)
        kprintf("sched_alarm(%x, %d.%d)\n", tcb->mypid, sec, nsec);
#endif
    pid_t pid = tcb->mypid, next=ccb.hd_timerq, prev=0;
    /* ccb.hd_active -> ccb.hd_blocked */
    sched_block(tcb);
    // restore mapping
    tcb = sched_get_tcb(pid);
    tcb->alarmsec = sec;
    tcb->alarmns = nsec;
    /* add to begining of the queue */
    if(ccb.hd_timerq==0 || sec<tcba->alarmsec || (sec==tcba->alarmsec && nsec<tcba->alarmns)) {
        tcb->alarm = ccb.hd_timerq;
        ccb.hd_timerq = pid;
        // map the tcb at the head of the queue for easy access
        kmap((uint64_t)&tmpalarm, (uint64_t)(ccb.hd_timerq * __PAGESIZE), PG_CORE_NOCACHE);
    } else {
        /* walk through ccb.hd_timerq queue */
        do {
            OSZ_tcb *t = sched_get_tcb(next);
            /* first thread that has to be awaken later than us? */
            if(t->alarmsec>sec || (t->alarmsec==sec && t->alarmns>nsec)){
                if(prev) {
                    t = sched_get_tcb(prev);
                    t->alarm = pid;
                }
                tcb = sched_get_tcb(pid);
                tcb->alarm = next;
                break;
            }
            /* end of the list? */
            if(t->alarm==0) {
                t->alarm = pid;
                break;
            }
            prev = next;
            next = t->alarm;
        } while(next!=0);
    }
}

// hybernate a thread
void sched_sleep(OSZ_tcb *tcb)
{
#if DEBUG
    if(sysinfostruc.debug&DBG_SCHED)
        kprintf("sched_sleep(%x)\n", tcb->mypid);
#endif
    /* TODO: memory -> swap (except tcb) */
    tcb->state = tcb_state_hybernated;
}

// awake a hybernated or blocked thread
void sched_awake(OSZ_tcb *tcb)
{
    pid_t pid = tcb->mypid, next = tcb->next, prev = tcb->prev;
#if DEBUG
    if(sysinfostruc.debug&DBG_SCHED)
        kprintf("sched_awake(%x)\n", tcb->mypid);
#endif
    if(tcb->state == tcb_state_hybernated) {
        /* TODO: swap -> memory */
        tcb->state = tcb_state_blocked;
    }
    if(tcb->state == tcb_state_blocked) {
        /* ccb.hd_blocked -> ccb.hd_active */
        tcb->blkcnt += tcb->blktime - sysinfostruc.ticks[TICKS_LO];
        sched_add(tcb);
        if(ccb.hd_blocked == pid) {
            ccb.hd_blocked = next;
        }
        if(prev != 0) {
            tcb = sched_get_tcb(prev);
            tcb->next = next;
        }
        if(next != 0) {
            tcb = sched_get_tcb(next);
            tcb->prev = prev;
        }
        tcb = sched_get_tcb(pid);
    }
    tcb->state = tcb_state_running;
    isr_next = tcb->memroot;
}

// add a thread to priority queue
void sched_add(OSZ_tcb *tcb)
{
    pid_t pid = tcb->mypid;
#if DEBUG
    if(sysinfostruc.debug&DBG_SCHED)
        kprintf("sched_add(%x) pri %d\n", pid, tcb->priority);
#endif
    tcb->prev = 0;
    tcb->next = ccb.hd_active[tcb->priority];
    ccb.hd_active[tcb->priority] = pid;
    if(tcb->next != 0) {
        tcb = sched_get_tcb(tcb->next);
        tcb->prev = pid;
        tcb = sched_get_tcb(pid);
    }
}

// remove a thread from priority queue
void sched_remove(OSZ_tcb *tcb)
{
    pid_t next = tcb->next, prev = tcb->prev;
#if DEBUG
    if(sysinfostruc.debug&DBG_SCHED)
        kprintf("sched_remove(%x) pri %d\n", tcb->mypid, tcb->priority);
#endif
    tcb->prev = tcb->next = 0;
    if(ccb.hd_active[tcb->priority] == tcb->mypid) {
        ccb.hd_active[tcb->priority] = next;
    }
    if(ccb.cr_active[tcb->priority] == tcb->mypid) {
        ccb.cr_active[tcb->priority] = next;
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
    /* never block the idle task */
    if(tcb->memroot == idle_mapping)
        return;
#if DEBUG
    if(sysinfostruc.debug&DBG_SCHED)
        kprintf("sched_block(%x)\n", tcb->mypid);
#endif
    tcb->blktime = sysinfostruc.ticks[TICKS_LO];
    tcb->state = tcb_state_blocked;
    /* ccb.hd_active -> ccb.hd_blocked */
    pid_t pid = tcb->mypid;
    sched_remove(tcb);
    // link into blocked queue
    if(ccb.hd_blocked!=0) {
        tcb = sched_get_tcb(ccb.hd_blocked);
        tcb->prev = pid;
    }
    // restore mapping
    tcb = sched_get_tcb(pid);
    // link as the first item in chain
    tcb->next = ccb.hd_blocked;
    tcb->prev = 0;
    tcb->state = tcb_state_blocked;
    ccb.hd_blocked = pid;
}

/* pick a thread and return it's memroot */
phy_t sched_pick()
{
    OSZ_tcb *tcb, *curr = (OSZ_tcb*)0;
    int i, nonempty = false;
again:
//kprintf("pick\n");
    /* iterate on priority queues. Note that this does not depend
     * on number of tasks, so this is a O(1) scheduler algorithm */
    for(i=PRI_SYS; i<PRI_IDLE; i++) {
//kprintf("  %d %x %x\n",i,ccb.hd_active[i],ccb.cr_active[i]);
        // skip empty queues
        if(ccb.hd_active[i] == 0)
            continue;
        // we've found at least one non-empty queue
        nonempty = true;
        //if we're on the end of the list, go to head
        if(ccb.cr_active[i] == 0) {
            ccb.cr_active[i] = ccb.hd_active[i];
            continue;
        }
        goto found;
    }
    /* we came here for two reasons:
     * 1. all queues turned around
     * 2. there are no threads to run
     * in the first case we choose the highest priority task, in the second
     * case we step to IDLE queue */
    if(nonempty)
        goto again;
    i = PRI_IDLE;
//kprintf("  %d %x %x\n",i,ccb.hd_active[i],ccb.cr_active[i]);
    /* if there's nothing to schedule, use idle task */
    if(ccb.hd_active[i]==0) {
#if DEBUG
        if(sysinfostruc.debug&DBG_SCHED && curr->memroot != idle_mapping)
            kprintf("sched_pick()=idle  \n");
#endif
        if(idle_first) {
            idle_first = false;
            sys_ready();
        }
        isr_next = idle_mapping;
        return idle_mapping;
    }
    //if we're on the end of the list, go to head
    if(ccb.cr_active[i] == 0)
        ccb.cr_active[i] = ccb.hd_active[i];
found:
//kprintf("found %d %x\n",i,ccb.cr_active[i]);
    tcb = sched_get_tcb(ccb.cr_active[i]);
    ccb.cr_active[i] = tcb->next;
    if(curr->mypid != tcb->mypid) {
#if DEBUG
        if(sysinfostruc.debug&DBG_SCHED)
            kprintf("sched_pick()=%x  \n", tcb->mypid);
#endif
        isr_next = tcb->memroot;
    }
    return tcb->memroot;
}
