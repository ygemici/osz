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
extern OSZ_pmm pmm;

uint64_t __attribute__ ((section (".data"))) core_mapping;
uint64_t __attribute__ ((section (".data"))) corepde_mapping;
uint64_t __attribute__ ((section (".data"))) *stack_ptr;

pid_t thread_new(char *cmdline)
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
    self = (uint64_t)ptr;
    tcb->allocmem = 8 + nrmqmax;
    tcb->evtq_size = (nrmqmax*__PAGESIZE)/sizeof(msg_t);
    tcb->cs = 0x20+3; // ring 3 user code
    tcb->ss = 0x18+3; // ring 3 user data
    tcb->rflags = 0x2; // enable interrupts and mandatory 2

    /* allocate memory mappings */
    // PML4
    ptr=pmm_alloc();
    tcb->memroot = (uint64_t)ptr;
    kmap((uint64_t)&tmp2map, (uint64_t)ptr, PG_CORE_NOCACHE);
    // PDPE
    ptr=pmm_alloc();
    paging[0]=(uint64_t)ptr+PG_USER_RW;
    paging[511]=core_mapping;
    kmap((uint64_t)&tmp2map, (uint64_t)ptr, PG_CORE_NOCACHE);
    // PDE
    ptr=pmm_alloc();
    paging[0]=(uint64_t)ptr+PG_USER_RW;
    kmap((uint64_t)&tmp2map, (uint64_t)ptr, PG_CORE_NOCACHE);
    // PT text
    ptr=pmm_alloc();
    paging[0]=(uint64_t)ptr+PG_USER_RW;
    ptr2=pmm_alloc();
    paging[1]=(uint64_t)ptr2+PG_USER_RW;
    kmap((uint64_t)&tmp2map, (uint64_t)ptr, PG_CORE_NOCACHE);
    // map TCB
    tcb->self = (uint64_t)ptr;
    paging[0]=self+PG_USER_RO;
    // allocate message queue
    for(i=0;i<nrmqmax;i++) {
        ptr=pmm_alloc();
        paging[i+(MQ_ADDRESS/__PAGESIZE)]=(uint64_t)ptr+PG_USER_RW;
    }
    // allocate stack
    ptr=pmm_alloc();
    paging[511]=(uint64_t)ptr+PG_USER_RW;
    // we don't need the table any longer, so map it to message queue header
    kmap((uint64_t)&tmp2map, (uint64_t)(paging[(MQ_ADDRESS/__PAGESIZE)]&~(__PAGESIZE-1)), PG_CORE_NOCACHE);
    eventhdr_t *evthdr = (eventhdr_t *)&tmp2map;
    evthdr->evtq_ptr = evthdr->evtq_endptr = (msg_t*)(MQ_ADDRESS+sizeof(eventhdr_t));
    evthdr->evtq_nextserial = 1;
    // set up stack
    kmap((uint64_t)&tmp2map, (uint64_t)ptr, PG_CORE_NOCACHE);
    i = 511-(kstrlen(cmdline)+2+7)/8;
    kstrcpy((char*)(&paging[i--]), cmdline);
    paging[i--] = 0;                                 // argv[1]
    paging[i] = TEXT_ADDRESS-__PAGESIZE+(i+2)*8;i--; // argv[0]
    paging[i] = TEXT_ADDRESS-__PAGESIZE+(i+1)*8;i--; // argv
    paging[i--] = 1;                                 // argc
    stack_ptr = ptr + i*8;
    tcb->rsp = TEXT_ADDRESS - __PAGESIZE + (i+1)*8;
    // map text segment mapping for elf loading
    kmap((uint64_t)&tmp2map, (uint64_t)ptr2, PG_CORE_NOCACHE);
#if DEBUG
    kprintf("tcb=%x stack=%x %s\n",tcb->self,stack_ptr,cmdline);
#endif
    return self/__PAGESIZE;
}

// map a thread
void thread_map(pid_t thread)
{
    OSZ_tcb *tcb = (OSZ_tcb*)(&tmp2map);
    kmap((uint64_t)&tmp2map, (uint64_t)(thread*__PAGESIZE), PG_CORE_NOCACHE);
    __asm__ __volatile__ ( "mov %0, %%rax; mov %%rax, %%cr3" : : "r"(tcb->memroot) : "%rax" );
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
