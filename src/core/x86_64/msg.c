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

/* send a message in registers */
bool_t msg_sendreg(pid_t thread, uint64_t event, uint64_t arg0, uint64_t arg1, uint64_t arg2)
{
    OSZ_tcb *srctcb = (OSZ_tcb*)(0);
    OSZ_tcb *dsttcb = (OSZ_tcb*)(&tmp2map);
    uint64_t *pde = (uint64_t*)(&tmppde);
    if(event==0)
        return false;
    // map thread's message queue at dst_mq
    kmap((uint64_t)&tmp2map, (uint64_t)(thread*__PAGESIZE), PG_CORE_NOCACHE);
    pde[DSTMQ_PDE] = dsttcb->self;
    __asm__ __volatile__ ( "invlpg dst_mq" : : :);
    // check if the dest is receiving from ANY (0) or from our pid
    if(dst_mq.mq_recvfrom!=0 && dst_mq.mq_recvfrom!=srctcb->mypid)
        return false;

    // send message to the mapped queue
    ksend(&dst_mq,
        MSG_DEST(thread) | MSG_REGDATA | MSG_TYPE(event),
        arg0,
        arg1,
        arg2
    );
    return true;
}

/* send a message in memory */
bool_t msg_sendptr(pid_t thread, uint64_t event, void *ptr, size_t size)
{
    OSZ_tcb *srctcb = (OSZ_tcb*)(0);
    OSZ_tcb *dsttcb = (OSZ_tcb*)(&tmp2map);
    uint64_t *pde = (uint64_t*)(&tmppde);
    if(event==0)
        return false;
    // map thread's message queue at dst_mq
    kmap((uint64_t)&tmp2map, (uint64_t)(thread*__PAGESIZE), PG_CORE_NOCACHE);
    pde[DSTMQ_PDE] = dsttcb->self;
    __asm__ __volatile__ ( "invlpg dst_mq" : : :);
    // check if the dest is receiving from ANY (0) or from our pid
    if(dst_mq.mq_recvfrom!=0 && dst_mq.mq_recvfrom!=srctcb->mypid)
        return false;
    // TODO: map or copy message to destination's address space
    // map ptr to local stack (tcb+mq+stack at dst_mq)

    // send message to the mapped queue
    ksend(&dst_mq,
        MSG_DEST(thread) | MSG_REGDATA | MSG_TYPE(event),
        (uint64_t)ptr,
        (uint64_t)size,
        0
    );
    return true;
}
