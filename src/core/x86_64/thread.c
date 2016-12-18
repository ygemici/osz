/*
 * core/x86_64/thread.c
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
 * @brief Thread functions platform dependent code
 */

#include "platform.h"
#include "../env.h"

/* external resources */
extern uint8_t MQ_ADDRESS;
extern OSZ_pmm pmm;

uint64_t __attribute__ ((section (".data"))) core_mapping;

pid_t thread_new()
{
    OSZ_tcb *tcb = (OSZ_tcb*)(pmm.bss_end);
    uint64_t *paging = (uint64_t *)&tmp2map, self;
    void *ptr, *ptr2;
    uint i;

    /* allocate at least 1 page for message queue */
    if(nrmqmax<1)
        nrmqmax=1;

    /* allocate TCB */
    ptr=pmm_alloc();
    kmap((uint64_t)pmm.bss_end, (uint64_t)ptr, PG_CORE_NOCACHE);
    tcb->magic = OSZ_TCB_MAGICH;
    tcb->state = tcb_running;
    tcb->priority = PRI_SRV;
    self = (uint64_t)ptr+1;
    tcb->allocmem = 7 + nrmqmax;
    tcb->evtq_ptr = tcb->evtq_endptr = (OSZ_event*)&MQ_ADDRESS;

    /* allocate memory mappings */
    // PML4
    ptr=pmm_alloc();
    tcb->memroot = (uint64_t)ptr;
    kmap((uint64_t)&tmp2map, (uint64_t)ptr, PG_CORE_NOCACHE);
    // PDPE
    ptr=pmm_alloc();
    paging[0]=(uint64_t)ptr+1;
    paging[511]=core_mapping;
    kmap((uint64_t)&tmp2map, (uint64_t)ptr, PG_CORE_NOCACHE);
    // PDE
    ptr=pmm_alloc();
    paging[0]=(uint64_t)ptr+1;
    kmap((uint64_t)&tmp2map, (uint64_t)ptr, PG_CORE_NOCACHE);
    // PT text
    ptr=pmm_alloc();
    paging[0]=(uint64_t)ptr+1;
    ptr2=pmm_alloc();
    paging[1]=(uint64_t)ptr2+1;
    kmap((uint64_t)&tmp2map, (uint64_t)ptr, PG_CORE_NOCACHE);
    // map TCB, relies on identity mapping
    tcb->self = (uint64_t)ptr;
    paging[0]=self;
    // allocate message queue
    for(i=0;i<nrmqmax;i++) {
        ptr=pmm_alloc();
        paging[i+((uint64_t)&MQ_ADDRESS/__PAGESIZE)]=(uint64_t)ptr+1;
    }
    // map text segment mapping for elf loading
    kmap((uint64_t)&tmp2map, (uint64_t)ptr2, PG_CORE_NOCACHE);
kprintf("tcb=%x\n",tcb->self);
    return self/__PAGESIZE;
}

// add a TCB to priority queue
void thread_add(pid_t thread)
{
    // uint64_t ptr = thread * __PAGESIZE;
}

// remove a TCB from priority queue
void thread_remove(pid_t thread)
{
    // uint64_t ptr = thread * __PAGESIZE;
}
