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
#include "platform.h"
#include "../env.h"
#include <errno.h>
#include <syscall.h>

extern uint64_t nrservices;
extern pid_t services[];

/* send a message with a memory reference. If given, magic is a type hint for ptr */
bool_t msg_send(pid_t thread, uint64_t event, void *ptr, size_t size, uint64_t magic)
{
    OSZ_tcb *srctcb = (OSZ_tcb*)(0);
    OSZ_tcb *dsttcb = (OSZ_tcb*)(&tmpmap);
    uint64_t *pde = (uint64_t*)(&tmppde);
    if(event==0) {
        srctcb->errno = EPERM;
        return false;
    }
    // do we need pid_t translation?
    if((int64_t)thread < 0) {
        if((int64_t)thread > SRV_usrfirst) {
            thread = subsystems[-((int64_t)thread)];
        } else {
            if(-((int64_t)thread+32) < nrservices) {
                thread = services[-((int64_t)thread+32)];
            } else {
                srctcb->errno = EINVAL;
                return false;
            }
        }
    }
    // map thread's message queue at tmpmq
    kmap((uint64_t)&tmpmap, (uint64_t)(thread*__PAGESIZE), PG_CORE_NOCACHE);
    pde[DSTMQ_PDE] = dsttcb->self;
    __asm__ __volatile__ ( "invlpg tmpmq" : : :);
    // check if the dest is receiving from ANY (0) or from our pid
    if(tmpmq.mq_recvfrom != 0 && tmpmq.mq_recvfrom != srctcb->mypid) {
        srctcb->errno = EAGAIN;
        return false;
    }
    // core memory cannot be sent in range -512M..0
    if((int64_t)ptr < 0 && (int64_t)ptr > (int64_t)(&fb)) {
        srctcb->errno = EACCES;
        return false;
    }
    // map ptr into destination's address space if it's not pointing
    // to shared memory (not in the range -2^56..-512M).
    if((int64_t)ptr > 0) {
        size_t s;
        void *p = ptr;
        // check size. Buffers over 1M must be sent in shared memory
        if(size > (__PAGESIZE<<8)) {
            srctcb->errno = EINVAL;
            return false;
        }
        // TODO: map ptr after mq (tcb + nrmqmax*__PAGESIZE) in destination
        // thread's memory, that would be &tmpmq[tmpmq[0].mq_size]
        for(s = size; s > 0; s -= __PAGESIZE) {
            /* uint64_t src=*(kmap_getpte(p)) */
/*    kmap((uint64_t)&tmpmap, (uint64_t)ptr, PG_CORE_NOCACHE); */

            p += __PAGESIZE;
        }
        // modify pointer to point into the newly mapped area
        ptr = (void*)((uint64_t)ptr & (__PAGESIZE-1));
        ptr += ((uint64_t)srctcb + nrmqmax*__PAGESIZE);
    }

    // send message to the mapped queue
    srctcb->errno = EBUSY;
    if(!ksend(&tmpmq,
        MSG_DEST(thread) | MSG_PTRDATA | MSG_FUNC(event),
        (uint64_t)ptr,
        (uint64_t)size,
        magic
    ))
        sched_block(0);
    else
        srctcb->errno = SUCCESS;
    return true;
}

/* send a message with scalar values */
bool_t msg_sends(pid_t thread, uint64_t event, uint64_t arg0, uint64_t arg1, uint64_t arg2)
{
    OSZ_tcb *srctcb = (OSZ_tcb*)(0);
    OSZ_tcb *dsttcb = (OSZ_tcb*)(&tmpmap);
    uint64_t *pde = (uint64_t*)(&tmppde);
    if(event==0) {
        srctcb->errno = EPERM;
        return false;
    }
    // do we need pid_t translation?
    if((int64_t)thread < 0) {
        if((int64_t)thread > SRV_usrfirst) {
            thread = subsystems[-((int64_t)thread)];
        } else {
            if(-((int64_t)thread+32) < nrservices) {
                thread = services[-((int64_t)thread+32)];
            } else {
                srctcb->errno = EINVAL;
                return false;
            }
        }
    }
    // map thread's message queue at tmpmq
    kmap((uint64_t)&tmpmap, (uint64_t)(thread*__PAGESIZE), PG_CORE_NOCACHE);
    pde[DSTMQ_PDE] = dsttcb->self;
    __asm__ __volatile__ ( "invlpg tmpmq" : : :);
    // check if the dest is receiving from ANY (0) or from our pid
    if(tmpmq.mq_recvfrom!=0 && tmpmq.mq_recvfrom!=srctcb->mypid) {
        srctcb->errno = EAGAIN;
        return false;
    }
    // send message to the mapped queue
    srctcb->errno = EBUSY;
    if(!ksend(&tmpmq,
        MSG_DEST(thread) | MSG_REGDATA | MSG_FUNC(event),
        arg0,
        arg1,
        arg2
    ))
        sched_block(0);
    else
        srctcb->errno = SUCCESS;
    return true;
}
