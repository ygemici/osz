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

#include <errno.h>
#include <syscall.h>
#include <sys/sysinfo.h>
#include "env.h"

extern sysinfo_t sysinfostruc;
extern uint64_t nrservices;
extern pid_t *services;
extern pid_t isr_next;

/* pointer to PDE for TMPQ_ADDRESS */
phy_t __attribute__ ((section (".data"))) *mq_mapping;

/* send a message */
uint64_t msg_sends(evt_t event, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5)
{
    OSZ_tcb *srctcb = (OSZ_tcb*)(0);
    OSZ_tcb *dsttcb = (OSZ_tcb*)(&tmpmap);
    msghdr_t *msghdr = (msghdr_t*)(TMPQ_ADDRESS + MQ_ADDRESS);
    uint64_t *paging = (uint64_t*)(&tmp2map);
    // don't use EVT_SENDER() here, would loose sign. This can be
    // negative pid if a service referenced.
    int64_t thread = ((int64_t)(event&~0xFFFF)/(int64_t)65536);

    // only ISRs allowed to send SYS_IRQ message, and they are sent
    // on a more efficient way, directly with ksend
    if(EVT_FUNC(event)==0) {
        srctcb->errno = EPERM;
        return false;
    }
    // do we need pid_t translation?
    if(thread < 0) {
        if(-thread < nrservices) {
            thread = services[-thread];
        } else {
            srctcb->errno = EINVAL;
            return false;
        }
    }

    // map destination thread's message queue
    kmap((uint64_t)&tmpmap, (uint64_t)((thread!=0?thread:srctcb->mypid)*__PAGESIZE), PG_CORE_NOCACHE);
    if(dsttcb->magic != OSZ_TCB_MAGICH) {
        srctcb->errno = EINVAL;
        return false;
    }
    isr_next = dsttcb->memroot;
    kmap_mq(dsttcb->memroot);

    /* mappings:
     *  tmpmap: destination thread's tcb
     *  tmp2map: destination thread's message queue's PTE (paging)
     *  TMPQ_ADDRESS: destination thread's self (message queue) */

    // check if the dest is receiving from ANY (0) or from our pid
    if(msghdr->mq_recvfrom != 0 && msghdr->mq_recvfrom != srctcb->mypid) {
        srctcb->errno = EAGAIN;
        return false;
    }
    phy_t pte;
    size_t s;
    int bs = 0;
    void *p = (void *)(arg0 & ~(__PAGESIZE-1));
#if DEBUG
    if(sysinfostruc.debug&DBG_MSG) {
        kprintf(" msg pid %x sending to pid %x (%d), event #%x",
            srctcb->mypid, thread, msghdr->mq_start, EVT_FUNC(event));
        if(event & MSG_PTRDATA)
            kprintf(" *%x[%d] (%x)",arg0,arg1,arg2);
        else
            kprintf("(%x,%x,%x,%x)",arg0,arg1,arg2,arg3);
        kprintf("\n");
    }
#endif
    /* TODO: use mq_buffstart properly as a circular buffer */
    // we have to save message buffer's address first
    if(event & MSG_PTRDATA){
        bs = (arg1+__PAGESIZE-1)/__PAGESIZE;
        // limit check
        if(msghdr->mq_buffstart/__PAGESIZE + bs >= nrmqmax + 1 + msghdr->mq_buffsize)
            msghdr->mq_buffstart = (nrmqmax + 1)*__PAGESIZE;
        // core memory cannot be sent in range -512M..0
        if(arg0!=(uint64_t)&sysinfostruc &&
            (int64_t)p < 0 && (uint64_t)p > FBUF_ADDRESS) {
            srctcb->errno = EACCES;
            return false;
        }
        // not mapping shared memory (not in the range -2^56..-512M).
        if((int64_t)p > 0 || (uint64_t)p == (uint64_t)&sysinfostruc) {
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

    srctcb->errno = EBUSY;
    // send message to the mapped queue. Don't use EVT_FUNC, would
    // loose MSG_PTRDATA flag
    if(!ksend(msghdr,
        EVT_DEST((uint64_t)thread) | (event&0xFFFF),
        arg0, arg1, arg2, arg3, arg4, arg5
    )) {
        if(srctcb->mypid!=dsttcb->mypid)
            sched_block(dsttcb);
        return false;
    } else {
        srctcb->errno = SUCCESS;
        if(srctcb->mypid!=dsttcb->mypid)
            sched_awake(dsttcb);
    }
    return true;
}
