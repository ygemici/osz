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

extern OSZ_ccb ccb;

OSZ_tcb *sched_get_tcb(pid_t thread)
{
    // uint64_t ptr = thread * __PAGESIZE;
    if(thread!=0) {
        // map tcb
        kmap((uint64_t)&tmpmap, (uint64_t)(thread * __PAGESIZE), PG_CORE_NOCACHE);
        return (OSZ_tcb*)(&tmpmap);
    }
    // current tcb
    return (OSZ_tcb*)0;
}

// add a TCB to priority queue
void sched_add(pid_t thread)
{
    // OSZ_tcb *tcb = sched_get_tcb(thread);
}

// remove a TCB from priority queue
void sched_remove(pid_t thread)
{
    // OSZ_tcb *tcb = sched_get_tcb(thread);
}

// move a TCB from priority queue to blocked queue
void sched_block(pid_t thread)
{
    // OSZ_tcb *tcb = sched_get_tcb(thread);
}

// move a TCB from blocked queue to priority queue
void sched_activate(pid_t thread)
{
    // uint64_t ptr = thread * __PAGESIZE;
}

// pick a thread and return it's memroot
uint64_t sched_pick()
{
    // uint64_t ptr = thread * __PAGESIZE;
    return 0;
}
