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

#include <platform.h>

// failsafe
#ifndef SYS_mountfs
#define SYS_mountfs 1
#endif

extern pid_t isr_next;

/* pointer to PDE for TMPQ_ADDRESS */
dataseg phy_t *mq_mapping;
/* errno in core */
dataseg uint64_t coreerrno;

/**
 * send a message with scalar values. Note msg_send() sends a buffer, it's a macro.
 */
uint64_t msg_sends(evt_t event, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5)
{
    tcb_t *srctcb = (tcb_t*)(0);
    tcb_t *dsttcb = (tcb_t*)(&tmpmap);
    msghdr_t *msghdr = (msghdr_t*)(TMPQ_ADDRESS + MQ_ADDRESS);
    uint64_t *paging = (uint64_t*)(&tmp2map);
    // don't use EVT_SENDER() here, would loose sign. This can be
    // negative pid if a service referenced.
    int64_t task = ((int64_t)(event&~0xFFFF)/(int64_t)65536);

    srand[event%4] *= srctcb->pid;
    kentropy();

    // only ISRs allowed to send SYS_IRQ message, and they are sent
    // on a more efficient way, directly with ksend
    if(EVT_FUNC(event)==0) {
        coreerrno = EPERM;
        return false;
    }
    // do we need pid_t translation?
    if(task < 0) {
        if(-task >= NUMSRV || services[-task]!=0) {
            task = services[-task];
        } else {
            coreerrno = EFAULT;
            return false;
        }
    }

    // checks
    if(task == services[-SRV_FS]) {
        // only init allowed to send SYS_mountfs to FS
        if(event==SYS_mountfs && srctcb->pid!=services[-SRV_init]) {
            coreerrno = EPERM;
            return false;
        }
        // only drivers allowed to send SYS_mknod to FS
        if(event==SYS_mknod && srctcb->priority!=PRI_DRV) {
            coreerrno = EPERM;
            return false;
        }
    }

    // map destination task's message queue
    kmap((uint64_t)&tmpmap, (uint64_t)((task!=0?task:srctcb->pid)*__PAGESIZE), PG_CORE_NOCACHE);
    if(dsttcb->magic != OSZ_TCB_MAGICH) {
        coreerrno = EINVAL;
        return false;
    }
    isr_next = dsttcb->memroot;
    kmap_mq(dsttcb->memroot);

    /* mappings:
     *  tmpmap: destination task's tcb
     *  tmp2map: destination task's message queue's PTE (paging)
     *  TMPQ_ADDRESS: destination task's self (message queue) */

    phy_t pte;
    size_t s;
    int bs = 0;
    void *p = (void *)(arg0 & ~(__PAGESIZE-1));
    if(debug&DBG_MSG) {
        kprintf(" msg pid %x sending to pid %x (%d), event #%x",
            srctcb->pid, task, msghdr->mq_start, EVT_FUNC(event));
        if(event & MSG_PTRDATA)
            kprintf(" *%x[%d] (%x)",arg0,arg1,arg2);
        else
            kprintf("(%x,%x,%x,%x)",arg0,arg1,arg2,arg3);
        kprintf("\n");
    }
    /* TODO: use mq_buffstart properly as a circular buffer */
    // we have to save message buffer's address first
    if(event & MSG_PTRDATA){
        bs = (arg1+__PAGESIZE-1)/__PAGESIZE;
        // limit check
        if(msghdr->mq_buffstart/__PAGESIZE + bs >= nrmqmax + 1 + msghdr->mq_buffsize)
            msghdr->mq_buffstart = (nrmqmax + 1)*__PAGESIZE;
        // core memory cannot be sent in range -64M..0
        if((int64_t)p < 0 && (uint64_t)p > FBUF_ADDRESS) {
            coreerrno = EACCES;
            return false;
        }
        // not mapping shared memory (not in the range -2^56..-64M).
        if((int64_t)p > 0) {
            // check size. Buffers over 1M must be sent in shared memory
            if(arg1 > (__SLOTSIZE/2)) {
                coreerrno = EINVAL;
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

    coreerrno = EBUSY;
    // send message to the mapped queue. Don't use EVT_FUNC, would
    // loose MSG_PTRDATA flag
    if(!ksend(msghdr,
        EVT_DEST((uint64_t)task) | (event&0xFFF),
        arg0, arg1, arg2, arg3, arg4, arg5
    )) {
        if(srctcb->pid!=dsttcb->pid)
            sched_block(dsttcb);
        return false;
    } else {
        coreerrno = SUCCESS;
        if(srctcb->pid!=dsttcb->pid)
            sched_awake(dsttcb);
    }
    return true;
}
