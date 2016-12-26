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

extern uint64_t nrservices;
extern pid_t services[];

/* array of physical page addresses for ptr bufferred messages */
uint64_t __attribute__ ((section (".data"))) *msgbuff;

/* send a message with a memory reference. If given, magic is a type hint for ptr */
bool_t msg_send(pid_t thread, uint64_t event, void *ptr, size_t size, uint64_t magic)
{
    OSZ_tcb *srctcb = (OSZ_tcb*)(0);
    OSZ_tcb *dsttcb = (OSZ_tcb*)(&tmpmap);
    msghdr_t *msghdr = (msghdr_t*)(&tmpmap);
    uint64_t *paging = (uint64_t*)(&tmpmap);
    uint64_t dstmemroot = 0, oldtcbphy = *(kmap_getpte((uint64_t)srctcb));

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
    // map thread's message queue
    kmap((uint64_t)&tmpmap, (uint64_t)(thread*__PAGESIZE), PG_CORE_NOCACHE);
    dstmemroot = dsttcb->memroot;
    kmap((uint64_t)&tmpmap, (uint64_t)(dsttcb->self), PG_CORE_NOCACHE);
    kmap((uint64_t)&tmpmap, (uint64_t)(paging[MQ_ADDRESS/__PAGESIZE]), PG_CORE_NOCACHE);

//    pde[DSTMQ_PDE] = dsttcb->self;
//    __asm__ __volatile__ ( "invlpg (tmpmq)" : : :);
#if DEBUG
    if(debug&DBG_MSG)
        kprintf(" msg %x sending to %x, event #%x \n",
            srctcb->mypid, thread, event);
#endif
    // check if the dest is receiving from ANY (0) or from our pid
    if(msghdr->mq_recvfrom != 0 && msghdr->mq_recvfrom != srctcb->mypid) {
        srctcb->errno = EAGAIN;
        return false;
    }
    // if destination's address space is different, map it
    if(dstmemroot != srctcb->memroot) {
        size_t s;
        int i = 0;
        void *p = ptr;
        uint64_t newaddr = (nrmqmax+1)*__PAGESIZE;
        // we have to save message buffer's address first
        if(MSG_ISPTR(event)){
            // core memory cannot be sent in range -512M..0
            if((int64_t)ptr < 0 && (uint64_t)ptr > (uint64_t)(&fb)) {
                srctcb->errno = EACCES;
                return false;
            }
            // no mapping shared memory (not in the range -2^56..-512M).
            if((int64_t)ptr > 0) {
                // check size. Buffers over 1M must be sent in shared memory
                if(size > (__SLOTSIZE/2)) {
                    srctcb->errno = EINVAL;
                    return false;
                }
                // get mapping for message buffer
                for(s = (size+__PAGESIZE-1)/__PAGESIZE; s > 0; s --) {
                    msgbuff[i++] = *(kmap_getpte((uint64_t)p));
                    p += __PAGESIZE;
                }
                // modify pointer to point into the temporary buffer area
                ptr = (void*)((uint64_t)ptr & (__PAGESIZE-1));
                ptr += newaddr;
            }
        }
        // switch task
        thread_map(dstmemroot);
        // map ptr into destination's address space
        if(i){
            newaddr += i*__PAGESIZE;
            while(i>0) {
                newaddr -= __PAGESIZE;
                kmap(newaddr, msgbuff[--i], PG_USER_RW);
            }
        }
    }
    // map source tcb in tmpmap (we've switched address space, and we
    // need to access it to store errno)
    kmap((uint64_t)&tmpmap, (uint64_t)(oldtcbphy), PG_CORE_NOCACHE);
    // NOTE: swapped dsttcb and srctcb!!!

breakpoint;
    // send message to the mapped queue
    dsttcb->errno = EBUSY;
    if(!ksend((msghdr_t*)MQ_ADDRESS,
        MSG_DEST(thread) | MSG_PTRDATA | MSG_FUNC(event),
        (uint64_t)ptr,
        (uint64_t)size,
        magic
    ))
        sched_block(dsttcb);
    else
        dsttcb->errno = SUCCESS;
    return true;
}

/* send a message with scalar values */
__attribute__ ((flatten)) bool_t msg_sends(pid_t thread, uint64_t event, uint64_t arg0, uint64_t arg1, uint64_t arg2)
{
    return msg_send(thread, MSG_FUNC(event), (void*)&arg0, (size_t)arg1, arg2);
}
