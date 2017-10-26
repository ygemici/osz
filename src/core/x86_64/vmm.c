/*
 * core/x86_64/vmm.c
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
 * @brief Virtual Memory Manager, on-demand paging
 */

#include "arch.h"

/* external resources */
extern phy_t idle_mapping, core_mapping, identity_mapping;
extern void idle();
phy_t pdpe;
uint64_t lastpt;

/**
 * Page fault handler, called by isrs.S
 */
void exc14pagefault(uint64_t excno, uint64_t rip, uint64_t rsp)
{
//#if DEBUG
//    dbg_enable(rip,rsp,"page fault");
//#else
    kpanic(" --- page fault %d ---",excno);
//#endif
}

/**
 * create a new address space, map it's vmm tables and init it's TCB
 */
tcb_t *vmm_newtask(char *cmdline, uint8_t prio)
{
    tcb_t *tcb = (tcb_t*)&tmpmap;
    uint64_t memroot, *vmm=(uint64_t*)(&tmpmqctrl+64), *paging = (uint64_t *)&tmpmap;
    void *ptr;
    uint i;

    lastpt=512;

    /* allocate at least 1 page for message queue */
    if(nrmqmax<1) nrmqmax=1;

    /* allocate memory mappings */
    // PML4
    memroot=(uint64_t)pmm_alloc(1);
    kmap((uint64_t)&tmpmap, memroot, PG_CORE_NOCACHE|PG_PAGE);
    // PDPE
    ptr=pmm_alloc(1);
    paging[0]=(uint64_t)ptr+(PG_USER_RW|PG_PAGE);
    paging[511]=(core_mapping&~(__PAGESIZE-1))+(PG_CORE|PG_PAGE);
    kmap((uint64_t)&tmpmap, (uint64_t)ptr, PG_CORE_NOCACHE|PG_PAGE);
    // PDE
    ptr=pmm_alloc(1);
    paging[0]=vmm[0]=(uint64_t)ptr+(PG_USER_RW|PG_PAGE);
    kmap((uint64_t)&tmp3map, (uint64_t)ptr, PG_CORE_NOCACHE|PG_PAGE);
    kmap((uint64_t)&tmpmap, (uint64_t)ptr, PG_CORE_NOCACHE|PG_PAGE);
    // PT text
    ptr=pmm_alloc(1);
    paging[0]=((uint64_t)ptr+(PG_USER_RW|PG_PAGE))|(1UL<<PG_NX_BIT);
    kmap(VMM_ADDRESS, (uint64_t)ptr, PG_CORE_NOCACHE|PG_PAGE);
    if(prio<=PRI_IDLE) {
        //page table for first 2M code block
//        ptr=pmm_alloc(1);
//        paging[1]=(uint64_t)ptr+(PG_USER_RW|PG_PAGE);
    }
    paging=(uint64_t*)VMM_ADDRESS;

    /* allocate TCB */
    ptr=pmm_alloc(1);
    paging[0]=(uint64_t)ptr|PG_USER_RO|PG_PAGE|(1UL<<PG_NX_BIT);
    kmap((uint64_t)&tmpmap, (uint64_t)ptr, PG_CORE_NOCACHE|PG_PAGE);
    tcb->magic = OSZ_TCB_MAGICH;
    tcb->memroot = memroot;
    tcb->state = tcb_state_running;
    tcb->priority = prio;
    tcb->self = vmm[0];
    tcb->pid = (pid_t)((uint64_t)ptr/__PAGESIZE);
    tcb->allocmem = 5;
    tcb->cs = 0x20+3; // ring 3 user code
    tcb->ss = 0x18+3; // ring 3 user data
    tcb->rflags = 0x202; // enable interrupts and mandatory bit 1
    //set IOPL=3 in rFlags to permit IO address space
    if(prio==PRI_DRV) tcb->rflags |= (3<<12);

    // allocate stack
    ptr=pmm_alloc(1);
    tcb->allocmem++;
    paging[511]=(uint64_t)ptr|PG_USER_RW|PG_PAGE|(1UL<<PG_NX_BIT);
    // set up stack, watch out for alignment
    kmap((uint64_t)&tmp2map, (uint64_t)ptr, PG_CORE_NOCACHE|PG_PAGE);
    paging = (uint64_t*)&tmp2map;
    i = 511-((kstrlen(cmdline)+2+15)/16*2);
    tcb->name = TEXT_ADDRESS - __PAGESIZE + i*8;
    kstrcpy((char*)(&paging[i--]), cmdline);
    paging[i--] = 0;                                 // argv[1]
    paging[i] = TEXT_ADDRESS-__PAGESIZE+(i+2)*8;i--; // argv[0]
    paging[i] = TEXT_ADDRESS-__PAGESIZE+(i+1)*8;i--; // argv
    paging[i--] = 1;                                 // argc
    tcb->rsp = tcb->cmdline = TEXT_ADDRESS - __PAGESIZE + (i+1)*8;
    if(prio>PRI_IDLE) return tcb;

    // allocate and set up message queue
    for(i=0;i<nrmqmax;i++) {
        ptr=pmm_alloc(1);
        paging[i+(MQ_ADDRESS/__PAGESIZE)]=(uint64_t)ptr|PG_USER_RO|PG_PAGE|(1UL<<PG_NX_BIT);
    }
    tcb->allocmem += nrmqmax;
    kmap((uint64_t)&tmp2map, (uint64_t)(paging[(MQ_ADDRESS/__PAGESIZE)]&~(__PAGESIZE-1)), PG_CORE_NOCACHE|PG_PAGE);
    msghdr_t *msghdr = (msghdr_t *)&tmp2map;
    msghdr->mq_start = msghdr->mq_end = 1;
    msghdr->mq_size = (nrmqmax*__PAGESIZE)/sizeof(msg_t);
    msghdr->mq_buffstart = msghdr->mq_buffend = msghdr->mq_buffmin = (nrmqmax+1);
    msghdr->mq_buffsize = __SLOTSIZE/__PAGESIZE/2;

#if DEBUG
    if(debug&DBG_TASKS)
        kprintf("Task %x memroot %x stack %x %s\n",tcb->pid,tcb->memroot,ptr,cmdline);
#endif
    return tcb;
}

/**
 * add address space for idle task
 */
tcb_t *vmm_idletask()
{
    tcb_t *tcb = vmm_newtask("idle", PRI_IDLE+1);
    //start executing a special function.
    tcb->rip = (uint64_t)&idle;
    tcb->cs = 0x8;  //ring 0 selectors
    tcb->ss = 0x10;
    return tcb;
}

/**
 * map an existing address space's vmm tables
 */
tcb_t *vmm_maptask(phy_t memroot)
{
    tcb_t *tcb = (tcb_t*)&tmpmap;
    uint64_t *vmm=(uint64_t*)&tmp3map, *paging = (uint64_t *)&tmpmap;
    kmap((uint64_t)&tmpmap, memroot, PG_CORE_NOCACHE|PG_PAGE);
    kmap((uint64_t)&tmpmap, paging[0], PG_CORE_NOCACHE|PG_PAGE);
    kmap((uint64_t)&tmp3map, paging[0], PG_CORE_NOCACHE|PG_PAGE);
    kmap((uint64_t)&tmpmap, paging[0], PG_CORE_NOCACHE|PG_PAGE);
    kmap(VMM_ADDRESS, paging[0], PG_CORE_NOCACHE|PG_PAGE);
    kmap((uint64_t)&tmpmap, paging[0], PG_CORE_NOCACHE|PG_PAGE);
    paging=(uint64_t*)VMM_ADDRESS;
    lastpt=511;
    do {
        lastpt++;
        if(vmm[lastpt/512]==0) {
            vmm[lastpt/512]=(uint64_t)pmm_alloc(1)|PG_CORE_NOCACHE|PG_PAGE;
            asm volatile( "invlpg (%0)" : : "b"(VMM_ADDRESS+lastpt*__PAGESIZE) : "memory" );
            tcb->allocmem++;
        }
    } while(paging[lastpt]!=0);
    return tcb;
}

/**
 * Set up the main entry point
 */
void vmm_setip(virt_t func)
{
    tcb_t *tcb = (tcb_t*)&tmpmap;
    if(tcb->rip==0) {
        tcb->rip=func;
kprintf("setip %x\n",func);
}
}

/**
 * Add and entry point to stack
 */
void vmm_pushentry(virt_t func)
{
    tcb_t *tcb = (tcb_t*)&tmpmap;
    tcb->rsp-=8;
    *((uint64_t*)tcb->rsp) = func;
kprintf("pushentry %x\n",func);
}

/**
 * Check address space consistency
 */
bool_t vmm_check()
{
    return true;
}

/**
 * add a new page to vmm tables if necessary
 */
void vmm_checklastpt()
{
    tcb_t *tcb = (tcb_t*)&tmpmap;
    uint64_t *vmm=(uint64_t*)&tmp3map;
    if(vmm[lastpt/512]==0) {
        vmm[lastpt/512]=(uint64_t)pmm_alloc(1)|PG_CORE_NOCACHE|PG_PAGE;
        asm volatile( "invlpg (%0)" : : "b"(VMM_ADDRESS+lastpt*__PAGESIZE) : "memory" );
        tcb->allocmem++;
    }
}

/**
 * Map a core buffer into task's address space who's vmm tables mapped with vmm_maptask
 */
virt_t vmm_mapbuf(void *buf, uint64_t npages, uint64_t access)
{
    int i=0;
    tcb_t *tcb = (tcb_t*)&tmpmap;
    uint64_t *paging = (uint64_t *)VMM_ADDRESS;
    for(i=0;i<npages;i++) {
        vmm_checklastpt();
        paging[lastpt++]=((*((uint64_t*)kmap_getpte((uint64_t)buf+i*__PAGESIZE)))&~(__PAGESIZE-1))|
            PG_PAGE|access|(access==PG_USER_RW?1UL<<PG_NX_BIT:0);
        tcb->linkmem++;
    }
    return (lastpt-npages)*__PAGESIZE;
}


/**
 * map memory into task's address space, regardless to vmm tables
 */
void vmm_mapbss(tcb_t *tcb, virt_t bss, phy_t phys, size_t size, uint64_t access)
{
    if(size==0)
        return;
    uint64_t *paging = (uint64_t *)&tmp3map;
    int i;
    phys &= ~(__PAGESIZE-1);
    access &= (__PAGESIZE-1+(1UL<<63));
    if(access==0)
        access=PG_USER_RW;
    // mark writable pages no execute too
    if(access & 2)
        access |= (1UL<<PG_NX_BIT);
#if DEBUG
    if(debug&DBG_VMM)
        kprintf("    vmm_mapbss(%x,%x,%x,%d,%x)\n", tcb->memroot, bss, phys, size, access);
#endif
again:
    kmap((uint64_t)&tmp3map, tcb->memroot, PG_CORE_NOCACHE|PG_PAGE);
    //PML4E
    i=(bss>>(12+9+9+9))&0x1FF;
    if((paging[i]&~(__PAGESIZE-1))==0) {
        paging[i]=(uint64_t)pmm_alloc(1) + (access|PG_PAGE);
        tcb->allocmem++;
    }
    pdpe=(phy_t)(paging[i] & ~(__PAGESIZE-1));
    kmap((uint64_t)&tmp3map, paging[i], PG_CORE_NOCACHE|PG_PAGE);
    //PDPE
    i=(bss>>(12+9+9))&0x1FF;
    pdpe+=i*8;
    if((paging[i]&~(__PAGESIZE-1))==0) {
        paging[i]=(uint64_t)pmm_alloc(1) + (access|PG_PAGE);
        tcb->allocmem++;
    }
    kmap((uint64_t)&tmp3map, paging[i], PG_CORE_NOCACHE|PG_PAGE);
    //PDE
    i=(bss>>(12+9))&0x1FF;
    if((paging[i]&~(__PAGESIZE-1))==0) {
        //map 2M at once if it's aligned and big
        if(phys&~((1<<(12+9))-1) && size>=__SLOTSIZE) {
            //fill PD records
            size=((size+__SLOTSIZE-1)/__SLOTSIZE)*__SLOTSIZE;
            while(size>0) {
                paging[i] = (uint64_t)phys | access | PG_SLOT | (1UL<<PG_NX_BIT);
                asm volatile( "invlpg (%0)" : : "b"(bss) : "memory" );
                bss  += __SLOTSIZE;
                phys += __SLOTSIZE;
                size -= __SLOTSIZE;
                tcb->linkmem += __SLOTSIZE/__PAGESIZE;
                i++; if(i>511) goto again;
            }
            return;
        }
        paging[i]=(uint64_t)pmm_alloc(1) + access;
        tcb->allocmem++;
    }
    kmap((uint64_t)&tmp3map, paging[i], PG_CORE_NOCACHE|PG_PAGE);
    //PTE
    i=(bss>>(12))&0x1FF;
    //fill PT records
    size=((size+__PAGESIZE-1)/__PAGESIZE)*__PAGESIZE;
    while(size>0) {
        paging[i] = (uint64_t)phys | access | PG_PAGE | (1UL<<PG_NX_BIT);
        asm volatile( "invlpg (%0)" : : "b"(bss) : "memory" );
        bss  += __PAGESIZE;
        phys += __PAGESIZE;
        size -= __PAGESIZE;
        tcb->linkmem++;
        i++; if(i>511) goto again;
    }
}

/**
 * unmap memory from task's address space
 */
void vmm_unmapbss(tcb_t *tcb, virt_t bss, size_t size)
{
    if(size==0)
        return;
    uint64_t *paging = (uint64_t *)&tmpmap;
    uint64_t *paging2 = (uint64_t *)&tmp2map;
    uint64_t *paging3 = (uint64_t *)&tmp3map;
    int i,j,k,sk,sj;
    bss &= ~(__PAGESIZE-1);
    size=((size+__PAGESIZE-1)/__PAGESIZE)*__PAGESIZE;
#if DEBUG
    if(debug&DBG_VMM)
        kprintf("    vmm_unmapbss(%x,%x,%d)\n", tcb->memroot, bss, size);
#endif
again:
    kmap((uint64_t)&tmpmap, tcb->memroot, PG_CORE_NOCACHE|PG_PAGE);
    //PML4E
    i=(bss>>(12+9+9+9))&0x1FF;
    kmap((uint64_t)&tmpmap, paging[i], PG_CORE_NOCACHE|PG_PAGE);
    //PDPE
    i=(bss>>(12+9+9))&0x1FF;
    kmap((uint64_t)&tmpmap, paging[i], PG_CORE_NOCACHE|PG_PAGE);
    //PDE
    i=(bss>>(12+9))&0x1FF;
    if(paging[i]&PG_SLOT) {
        while(size>0) {
            if(paging[i]&1){
                pmm_free((phy_t)(paging[i]&~(__PAGESIZE-1+(1UL<<63))), __SLOTSIZE/__PAGESIZE);
                paging[i] = 0;
                tcb->linkmem -= __SLOTSIZE/__PAGESIZE;
                asm volatile( "invlpg (%0)" : : "b"(bss) : "memory" );
            }
            bss  += __SLOTSIZE;
            size -= __SLOTSIZE;
            i++; if(i>511) goto again;
        }
    }
    //PTE
    kmap((uint64_t)&tmpmap, paging[i], PG_CORE_NOCACHE|PG_PAGE);
    i=(bss>>(12))&0x1FF;
    while(size>0) {
        if(paging[i]&1) {
            pmm_free((phy_t)(paging[i]&~(__PAGESIZE-1+(1UL<<63))), 1);
            paging[i] = 0;
            tcb->linkmem--;
            asm volatile( "invlpg (%0)" : : "b"(bss) : "memory" );
        }
        bss  += __PAGESIZE;
        size -= __PAGESIZE;
        i++; if(i>511) goto again;
    }

    //free mapping pages
    kmap((uint64_t)&tmpmap, tcb->memroot, PG_CORE_NOCACHE|PG_PAGE);
    //PML4E
    i=(bss>>(12+9+9+9))&0x1FF;
    kmap((uint64_t)&tmpmap, paging[i], PG_CORE_NOCACHE|PG_PAGE);
    //PDPE
    for(i=0;i<512;i++) {
        if(paging[i]&1) {
            //PDE
            kmap((uint64_t)&tmp2map, paging[i], PG_CORE_NOCACHE|PG_PAGE);
            sj=0;
            for(j=0;j<512;j++) {
                if(paging2[j]&1 && (paging2[j]&PG_SLOT)==0) {
                    //PTE
                    kmap((uint64_t)&tmp3map, paging2[j], PG_CORE_NOCACHE|PG_PAGE);
                    sk=0;for(k=0;k<512;k++) if(paging3[k]&1) sk++;
                    if(sk==0) {
                        pmm_free((phy_t)(paging2[j]&~(__PAGESIZE-1+(1UL<<63))), 1);
                        paging2[j]=0;
                    } else
                        sj++;
                }
            }
            if(sj==0) {
                pmm_free((phy_t)(paging[i]&~(__PAGESIZE-1+(1UL<<63))), 1);
                paging[i]=0;
            }
        }
    }
}
