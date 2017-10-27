/*
 * core/sched.c
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
 * @brief Task scheduler
 */

#include <arch.h>

extern ccb_t ccb;               //CPU Control Block (platform specific)
extern uint8_t idle_first;      //flag to indicate first idle schedule

uint64_t sched_next;            //next task to map when isr finishes

/**
 * get and map a TCB. You can also pass a TCB as input for
 * performance reasons. Also, vmm_map(tmpmap) is pre-cached.
 */
tcb_t *sched_get_tcb(pid_t task)
{
    // last mapped or last used tcb
    if((uint64_t)task==(uint64_t)&tmpmap ||
       (uint64_t)task==(uint64_t)&tmpalarm ||
       (uint64_t)task==(uint64_t)&tmpfx)
        return (tcb_t*)(task);
    // active tcb
    if(task==0)
        task=((tcb_t*)task)->pid;
    // map a new tcb
    vmm_map((uint64_t)&tmpmap, (uint64_t)(task * __PAGESIZE), PG_CORE_NOCACHE|PG_PAGE);
    return (tcb_t*)(&tmpmap);
}

#if DEBUG
void sched_dump()
{
    tcb_t *tcb, *curr = (tcb_t*)0;
    int i;
    pid_t p;
    for(i=PRI_SYS; i<=PRI_IDLE; i++) {
        kprintf("active %d hd %x:   ",i,ccb.hd_active[i]);
        p=ccb.hd_active[i];
        while(p!=0) {
            tcb=sched_get_tcb(p);
            kprintf("%s%x%s(%x,%x) ",
                tcb->pid==ccb.cr_active[i]?">":"",
                tcb->pid,
                tcb->pid==curr->pid?"*":"",
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
            tcb->pid,
            tcb->pid==curr->pid?"*":"",
            tcb->prev,tcb->next);
        p=tcb->next;
    }
    kprintf("\n");
    kprintf("timerq hd %x:   ",ccb.hd_timerq);
    p=ccb.hd_timerq;
    while(p!=0) {
        tcb=sched_get_tcb(p);
        kprintf("%x%s(%x) ",
            tcb->pid,
            tcb->pid==curr->pid?"*":"",
            tcb->alarm);
        p=tcb->alarm;
    }
    kprintf("\n\n");
}
#endif

/**
 * block until alarm, add task to ccb.hd_timerq list too
 * note that isr_timer() consumes tasks from timer queue this is only called if sleep time is
 * bigger than alarmstep, shorter usleeps are implemented as busy loops in libc
 */
void sched_alarm(tcb_t *tcb, uint64_t sec, uint64_t nsec)
{
    tcb_t *tcba = (tcb_t*)&tmpalarm;
    /* if alarm time is in the past, make sure it's running and return */
    if(ticks[TICKS_TS]>sec || (ticks[TICKS_TS]==sec && ticks[TICKS_NTS]>nsec)) {
        sched_awake(tcb);
        return;
    }
#if DEBUG
    if(debug&DBG_SCHED)
        kprintf("sched_alarm(%x, %d.%d)\n", tcb->pid, sec, nsec);
#endif
    pid_t pid = tcb->pid, next=ccb.hd_timerq, prev=0;
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
        vmm_map((uint64_t)&tmpalarm, (uint64_t)(ccb.hd_timerq * __PAGESIZE), PG_CORE_NOCACHE|PG_PAGE);
    } else {
        /* walk through ccb.hd_timerq queue */
        do {
            tcb_t *t = sched_get_tcb(next);
            /* first task that has to be awaken later than us? */
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

/**
 * write out task's pages to swap, only keep it's TCB in blocked queue
 *  tcb_state_blocked -> tcb_state_hybernated
 */
void sched_hybernate(tcb_t *tcb)
{
#if DEBUG
    if(debug&DBG_SCHED)
        kprintf("sched_sleep(%x)\n", tcb->pid);
#endif
    /* TODO: memory -> swap (except tcb) */
    tcb->state = tcb_state_hybernated;
}

/**
 * awake a hybernated or blocked task
 *  tcb_state_blocked -> tcb_state_running
 */
void sched_awake(tcb_t *tcb)
{
    pid_t pid = tcb->pid, next = tcb->next, prev = tcb->prev;
#if DEBUG
    if(debug&DBG_SCHED)
        kprintf("sched_awake(%x)\n", tcb->pid);
#endif
    if(tcb->state == tcb_state_hybernated) {
        /* TODO: swap -> memory */
        tcb->state = tcb_state_blocked;
    }
    if(tcb->state == tcb_state_blocked) {
        /* ccb.hd_blocked -> ccb.hd_active */
        tcb->blkcnt += ticks[TICKS_LO] - tcb->blktime;
        tcb->blktime = 0;
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
    sched_next = tcb->memroot;
}

/**
 * add a task to an active queue, according to it's priority.
 */
void sched_add(tcb_t *tcb)
{
    pid_t pid = tcb->pid;
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
        tcb = sched_get_tcb(pid);
    }
}

/**
 * remove a task from active queue.
 */
void sched_remove(tcb_t *tcb)
{
    pid_t next = tcb->next, prev = tcb->prev;
#if DEBUG
    if(debug&DBG_SCHED)
        kprintf("sched_remove(%x) pri %d\n", tcb->pid, tcb->priority);
#endif
    tcb->prev = tcb->next = 0;
    if(ccb.hd_active[tcb->priority] == tcb->pid) {
        ccb.hd_active[tcb->priority] = next;
    }
    if(ccb.cr_active[tcb->priority] == tcb->pid) {
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

/**
 * move a TCB from active queue to blocked queue
 *  tcb_state_running -> txb_state_blocked
 */
void sched_block(tcb_t *tcb)
{
    /* never block the idle task */
    if(tcb->memroot == idle_mapping)
        return;
#if DEBUG
    if(debug&DBG_SCHED)
        kprintf("sched_block(%x)\n", tcb->pid);
#endif
    tcb->blktime = ticks[TICKS_LO];
    tcb->state = tcb_state_blocked;
    /* ccb.hd_active -> ccb.hd_blocked */
    pid_t pid = tcb->pid;
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

/**
 * pick a task from one of the active queues and return it's memroot 
 */
phy_t sched_pick()
{
    tcb_t *tcb, *curr = (tcb_t*)0;
    int i, nonempty = false;
again:
    /* iterate on priority queues. Note that this does not depend
     * on number of tasks, so this is a O(1) scheduler algorithm */
    for(i=PRI_SYS; i<PRI_IDLE; i++) {
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
     * 2. there are no tasks to run
     * in the first case we choose the highest priority task to run,
     * in the second case we step to IDLE queue */
    if(nonempty)
        goto again;

    i = PRI_IDLE;
    /* if there's nothing to schedule, use idle task */
    if(ccb.hd_active[i]==0) {
#if DEBUG
        if(debug&DBG_SCHED && curr->memroot != idle_mapping)
            kprintf("sched_pick()=idle  \n");
#endif
        sched_next = idle_mapping;
        /* if this is the first time we schedule idle, call sys_ready() */
        if(idle_first)
            sys_ready();
        return sched_next;
    }
    /* if we're on the end of the list, go to head */
    if(ccb.cr_active[i] == 0)
        ccb.cr_active[i] = ccb.hd_active[i];
found:
    tcb = sched_get_tcb(ccb.cr_active[i]);
    ccb.cr_active[i] = tcb->next;

    if(curr->pid != tcb->pid) {
#if DEBUG
        if(debug&DBG_SCHED)
            kprintf("sched_pick()=%x  \n", tcb->pid);
#endif
        sched_next = tcb->memroot;
    }
    return tcb->memroot;
}
