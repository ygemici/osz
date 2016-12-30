/*
 * core/msg.c
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
 * @brief Message sender routines
 */

#include "env.h"
#include <errno.h>
#include <syscall.h>

extern sysinfo_t *sysinfostruc;
extern uint64_t nrservices;
extern pid_t *services;

/* send a message with scalar values */
__attribute__ ((flatten)) bool_t msg_send(pid_t thread, uint64_t event, void *ptr, size_t size, uint64_t magic)
{
    return msg_sends(thread, MSG_PTRDATA | MSG_FUNC(event), (uint64_t)ptr, (uint64_t)size, magic, 0, 0, 0);
}
/* send a message with a memory reference. If given, magic is a type hint for ptr */
__inline__ inline bool_t msg_sends(pid_t thread, uint64_t event, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5)
{
    OSZ_tcb *srctcb = (OSZ_tcb*)(0);
    OSZ_tcb *dsttcb = (OSZ_tcb*)(&tmpmap);
    msghdr_t *msghdr = (msghdr_t*)(&tmp2map);
    uint64_t *paging = (uint64_t*)(&tmpmap);
    uint64_t dstmemroot = 0, oldtcbphy = srctcb->mypid*__PAGESIZE;

    // only ISRs allowed to send SYS_IRQ message
    if(MSG_FUNC(event)==0) {
        srctcb->errno = EPERM;
        return false;
    }
    // do we need pid_t translation?
    if((int64_t)thread < 0) {
        if(-(int64_t)thread < nrservices) {
            thread = services[-((int64_t)thread)];
        } else {
            srctcb->errno = EINVAL;
            return false;
        }
    }
#if DEBUG
    if(debug&DBG_MSG) {
        kprintf(" msg pid %x sending to pid %x, event #%x",
            srctcb->mypid, thread, MSG_FUNC(event));
        if(event & MSG_PTRDATA)
            kprintf(" %x *%x[%d]",arg2,arg0,arg1);
        else
            kprintf("(%x,%x,%x,%x) %x",arg0,arg1,arg2,arg3);
        kprintf("\n");
    }
#endif
    // map thread's message queue
    kmap((uint64_t)&tmpmap, (uint64_t)(thread>0?thread*__PAGESIZE:oldtcbphy), PG_CORE_NOCACHE);
    dstmemroot = dsttcb->memroot;
    kmap((uint64_t)&tmpmap, (uint64_t)(dsttcb->self), PG_CORE_NOCACHE);
    kmap((uint64_t)&tmp2map, (uint64_t)(paging[MQ_ADDRESS/__PAGESIZE]), PG_CORE_NOCACHE);

//    pde[DSTMQ_PDE] = dsttcb->self;
//    __asm__ __volatile__ ( "invlpg (tmpmq)" : : :);
    // check if the dest is receiving from ANY (0) or from our pid
    if(msghdr->mq_recvfrom != 0 && msghdr->mq_recvfrom != srctcb->mypid) {
        srctcb->errno = EAGAIN;
        return false;
    }
// if destination's address space is different, map it
    phy_t pte;
    size_t s;
    int bs = 0;
    void *p = (void *)(arg0 & ~(__PAGESIZE-1));
    /* TODO: use mq_buffstart properly as a circular buffer */
    // we have to save message buffer's address first
    if(event & MSG_PTRDATA){
        bs = (arg1+__PAGESIZE-1)/__PAGESIZE;
        // limit check
        if(msghdr->mq_buffstart/__PAGESIZE + bs >= nrmqmax + 1 + msghdr->mq_buffsize)
            msghdr->mq_buffstart = (nrmqmax + 1)*__PAGESIZE;
        // core memory cannot be sent in range -512M..0
        if(arg0!=(uint64_t)sysinfostruc &&
            (int64_t)p < 0 && (uint64_t)p > (uint64_t)(&fb)) {
            srctcb->errno = EACCES;
            return false;
        }
        // not mapping shared memory (not in the range -2^56..-512M).
        if((int64_t)p > 0 || (uint64_t)p == (uint64_t)sysinfostruc) {
            // check size. Buffers over 1M must be sent in shared memory
            if(arg1 > (__SLOTSIZE/2)) {
                srctcb->errno = EINVAL;
                return false;
            }
            // modify pointer to point into the temporary buffer area
            arg0 = arg0 & (__PAGESIZE-1);
            arg0 += msghdr->mq_buffstart*__PAGESIZE;
            // get mapping for message buffer
            for(s = bs; s > 0; s --) {
                pte = *kmap_getpte((uint64_t)p);
                paging[msghdr->mq_buffstart] = (pte & ~(__PAGESIZE-1)) + PG_USER_RO;
                p += __PAGESIZE;
                msghdr->mq_buffstart++;
            }
        }
    }
    // switch task
    thread_map(dstmemroot);
    // map source tcb in tmpmap (we've switched address space, and we
    // need to access it to store errno)
    kmap((uint64_t)&tmpmap, (uint64_t)(oldtcbphy), PG_CORE_NOCACHE);
    // NOTE: swapped dsttcb and srctcb!!!

    // send message to the mapped queue
    dsttcb->errno = EBUSY;
    if(!ksend((msghdr_t*)MQ_ADDRESS,
        MSG_DEST(thread) | (event&0xFFFF),
        arg0, arg1, arg2, arg3, arg4, arg5
    )) {
        if(thread==0)
            dsttcb->errno = EAGAIN;
        else
            sched_block(dsttcb);
        return false;
    }else
        dsttcb->errno = SUCCESS;
    return true;
}
