/*
 * core/aarch64/vmm.c
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
extern void idle();
extern uint64_t *vmm_tmp;
extern phy_t tmppte, identity_mapping, __data;
extern char kpanictlb[];
uint64_t lastpt;

/**
 * Page fault handler, called by isrs.S
 */
void exc14pagefault(uint64_t excno, uint64_t pc, uint64_t sp)
{
//#if DEBUG
//    dbg_enable(pc,sp,"page fault");
//#else
    kpanic(" --- page fault %d ---",excno);
//#endif
}

/**
 * Initialize virtual memory manager
 */
uint64_t *vmm_init()
{
    uint64_t *ptr, l2, l3;
    uint32_t i,j;
    asm volatile ("mrs %0, ttbr0_el1" : "=r" (identity_mapping));
    asm volatile ("mrs %0, ttbr1_el1" : "=r" (l2));
    l3= (0xFF << 0) |    // Attr=0: normal, IWBWA, OWBWA, NTR
        (0x04 << 8) |    // Attr=1: device, nGnRE (must be OSH too)
        (0x44 <<16);     // Attr=2: non cacheable
    asm volatile ("msr mair_el1, %0" : : "r" (l3));

    /* this is called very early. Relies on identity mapping
       to find the physical address of tmppte pointer in L3 */
    ptr=(uint64_t*)(l2&~0xFFF);
    // core L1
    ptr[511]&=~0xFFF; l2=ptr[511]; ptr[511]|=(1L<<61)|PG_CORE_NOCACHE|PG_PAGE; //APTable=1, no EL0 access
    // core L2
    ptr=(uint64_t*)l2;
    ptr[511]&=~0xFFF; l3=ptr[511]; ptr[511]|=PG_CORE_NOCACHE|PG_PAGE;
    // core L3
    ptr=(uint64_t*)l3;
    ptr[0]&=~0xFFF; ptr[0]|=(1L<<PG_NX_BIT)|PG_CORE|PG_PAGE;             // bootboot
    ptr[1]&=~0xFFF; ptr[1]|=(1L<<PG_NX_BIT)|PG_CORE|PG_PAGE;             // environment
    j=(uint32_t)(((uint64_t)&__data&0xfffff)>>12);
    for(i=2;i<j;i++) { ptr[i]&=~0xFFF; ptr[i]|=PG_CORE_RO|PG_PAGE; } // code
    for(i=j;ptr[i]&1;i++) { ptr[i]&=~0xFFF; ptr[i]|=(1L<<PG_NX_BIT)|PG_CORE_NOCACHE|PG_PAGE; } // data
    ptr[511]&=~0xFFF; ptr[511]|=(1L<<PG_NX_BIT)|PG_CORE_NOCACHE|PG_PAGE; // stack
    ptr[(((uint64_t)&tmpmqctrl)>>12)&0x1FF]=l2|(1L<<61)|PG_CORE_NOCACHE|PG_PAGE;
    ptr[(((uint64_t)&tmppte)>>12)&0x1FF]=l3|(1L<<61)|PG_CORE_NOCACHE|PG_PAGE;
    asm volatile ("dsb sy; tlbi vmalle1");
    asm volatile ("mrs %0, sctlr_el1" : "=r" (l3));
    l3|=(1<<19)|(1<<12)|(1<<2); // enable WXN, I instruction cache and C data cache
    asm volatile ("msr sctlr_el1, %0; isb" : : "r" (l3));
    return (void*)((uint64_t)&tmppte + (((((uint64_t)&tmpctrl)>>12)&0x1FF)<<3));
}

/**
 * private function
 */
uint64_t *vmm_getpte2(virt_t addr, phy_t memroot)
{
    phy_t page;
    int step=0;
    // failsafe
    if(addr>>48)
        asm volatile ("mrs %0, ttbr1_el1" : "=r" (memroot));
//kprintf("vmm_getpte %x %x\n",addr,memroot);
    *vmm_tmp=(memroot&0xFFFFFFFF000L)|PG_CORE_NOCACHE|PG_PAGE;
    asm volatile ("dsb ishst; tlbi vaae1, %0; dsb ish; isb" : : "r" ((virt_t)&tmpctrl>>12)); step++;
//asm volatile("at s1e1r, %1; mrs %1, par_el1":"=r"(page):"r"(&tmpctrl));kprintf("at s1e1r; par_el1 %x\n",page);
    page=(uint64_t)&tmpctrl|(((addr>>(12+9+9))&0x1FF)<<3);
    if(!(*((uint64_t*)page)&1)) goto err;
    *vmm_tmp=(*((uint64_t*)page)&0xFFFFFFFF000L)|PG_CORE_NOCACHE|PG_PAGE;
    asm volatile ("dsb ishst; tlbi vaae1, %0; dsb ish; isb" : : "r" ((virt_t)&tmpctrl>>12)); step++;
//asm volatile("at s1e1r, %1; mrs %1, par_el1":"=r"(page):"r"(&tmpctrl));kprintf("at s1e1r; par_el1 %x\n",page);
    page=(uint64_t)&tmpctrl|(((addr>>(12+9))&0x1FF)<<3);
    if(!(*((uint64_t*)page)&1)) {
err:    kpanic(kpanictlb,addr,step);
    }
    if((*((uint64_t*)page)&2)==0)
        return (uint64_t*)page;
    *vmm_tmp=(*((uint64_t*)page)&0xFFFFFFFF000L)|PG_CORE_NOCACHE|PG_PAGE;
    asm volatile ("dsb ishst; tlbi vaae1, %0; dsb ish; isb" : : "r" ((virt_t)&tmpctrl>>12)); step++;
//asm volatile("at s1e1r, %1; mrs %1, par_el1":"=r"(page):"r"(&tmpctrl));kprintf("at s1e1r; par_el1 %x\n",page);
    page=(uint64_t)&tmpctrl|(((addr>>12)&0x1FF)<<3);
//kprintf("vmm_getpte return %x %x\n",*vmm_tmp,page);
    return (uint64_t*)page;
}

/**
 * return paging table entry for virtual address. Similar to
 * the AT command, but does not return physical address rather
 * a pointer to the physical address
 */
phy_t *vmm_getpte(virt_t addr)
{
    uint64_t memroot;
    if(addr>>48)
        asm volatile ("mrs %0, ttbr1_el1" : "=r" (memroot));
    else
        asm volatile ("mrs %0, ttbr0_el1" : "=r" (memroot));
    return vmm_getpte2(addr,memroot);
}

/**
 * map a physical page into virtual address space
 */
void vmm_map(virt_t virt, phy_t phys, uint16_t access)
{
    uint64_t *ptr;
    // shortcuts for speed up
    if(virt==(virt_t)&tmpmap)   ptr=(uint64_t*)&vmm_tmp[3]; else
    if(virt==(virt_t)&tmp2map)  ptr=(uint64_t*)&vmm_tmp[4]; else
    if(virt==(virt_t)&tmpalarm) ptr=(uint64_t*)&vmm_tmp[5]; else
    if(virt==(virt_t)&tmpfx)    ptr=(uint64_t*)&vmm_tmp[6]; else
        ptr=vmm_getpte(virt);
    /* negate RO bit to NX bit */
    *ptr=(phys&0x0000FFFFFFFFF000)|access|((access>>7)&1?0:(1L<<PG_NX_BIT));
    asm volatile ("dsb ishst; tlbi vaae1, %0; dc cvau, %1; dsb ish; isb" : : "r" (virt>>12), "r"(virt));
//kprintf("vmm_map(%x,%x,%x) %x=%x\n",virt, phys, access, ptr, *ptr);
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
//kprintf("vmm_newtask %s\n",cmdline);

    /* allocate at least 1 page for message queue */
    if(nrmqmax<1) nrmqmax=1;

    /* allocate memory mappings */
    // L1
    memroot=(uint64_t)pmm_alloc(1);
    vmm_map((uint64_t)&tmpmap, memroot, PG_CORE_NOCACHE|PG_PAGE);
    // L2
    ptr=pmm_alloc(1);
    paging[0]=vmm[0]=(uint64_t)ptr+(PG_USER_RW|PG_PAGE);
    vmm_map((uint64_t)&tmp3map, (uint64_t)ptr, PG_CORE_NOCACHE|PG_PAGE);
    vmm_map((uint64_t)&tmpmap, (uint64_t)ptr, PG_CORE_NOCACHE|PG_PAGE);
    // L3
    ptr=pmm_alloc(1);
    paging[0]=((uint64_t)ptr+(PG_USER_RW|PG_PAGE))|(1UL<<PG_NX_BIT);
    vmm_map(VMM_ADDRESS, (uint64_t)ptr, PG_CORE_NOCACHE|PG_PAGE);
    paging=(uint64_t*)VMM_ADDRESS;

    /* allocate TCB */
    ptr=pmm_alloc(1);
    paging[0]=(uint64_t)ptr|PG_USER_RO|PG_PAGE|(1UL<<PG_NX_BIT);

//temporarily map bootboot exception handler
    paging[0x7F]=0x7F000|PG_USER_RW|PG_PAGE;
    paging[0x80]=0x80000|PG_USER_RO|PG_PAGE;
    paging[0x81]=0x81000|PG_USER_RO|PG_PAGE;
    paging[0x82]=0x82000|PG_USER_RO|PG_PAGE;
    paging[0x83]=0x83000|PG_USER_RO|PG_PAGE;
    paging[0x84]=0x84000|PG_USER_RO|PG_PAGE;
    paging[0x85]=0x85000|PG_USER_RO|PG_PAGE;

    vmm_map((uint64_t)&tmpmap, (uint64_t)ptr, PG_CORE_NOCACHE|PG_PAGE);
    tcb->magic = OSZ_TCB_MAGICH;
    tcb->memroot = memroot;
    tcb->state = tcb_state_running;
    tcb->priority = prio;
    tcb->self = vmm[0];
    tcb->pid = (pid_t)((uint64_t)ptr/__PAGESIZE);
    tcb->allocmem = 4;
/*
    tcb->cs = 0x20+3; // ring 3 user code
    tcb->ss = 0x18+3; // ring 3 user data
    tcb->rflags = 0x202; // enable interrupts and mandatory bit 1
    //set IOPL=3 in rFlags to permit IO address space
    if(prio==PRI_DRV) tcb->rflags |= (3<<12);
*/
    // allocate stack
    ptr=pmm_alloc(1);
    tcb->allocmem++;
    paging[511]=(uint64_t)ptr|PG_USER_RW|PG_PAGE|(1UL<<PG_NX_BIT);
    // set up stack, watch out for alignment
    vmm_map((uint64_t)&tmp2map, (uint64_t)ptr, PG_CORE_NOCACHE|PG_PAGE);
    paging = (uint64_t*)&tmp2map;
    i = 511-((kstrlen(cmdline)+2+15)/16*2);
    tcb->name = TEXT_ADDRESS - __PAGESIZE + i*8;
    kstrcpy((char*)(&paging[i--]), cmdline);
    paging[i--] = 0;                                 // argv[1]
    paging[i] = TEXT_ADDRESS-__PAGESIZE+(i+2)*8;i--; // argv[0]
    paging[i] = TEXT_ADDRESS-__PAGESIZE+(i+1)*8;i--; // argv
    paging[i--] = 1;                                 // argc
    tcb->sp = tcb->cmdline = TEXT_ADDRESS - __PAGESIZE + (i+1)*8;
    if(prio>PRI_IDLE) return tcb;

    // allocate and set up message queue
    for(i=0;i<nrmqmax;i++) {
        ptr=pmm_alloc(1);
        paging[i+(MQ_ADDRESS/__PAGESIZE)]=(uint64_t)ptr|PG_USER_RO|PG_PAGE|(1UL<<PG_NX_BIT);
    }
    tcb->allocmem += nrmqmax;
    vmm_map((uint64_t)&tmp2map, (uint64_t)(paging[(MQ_ADDRESS/__PAGESIZE)]&~(__PAGESIZE-1)), PG_CORE_NOCACHE|PG_PAGE);
    msghdr_t *msghdr = (msghdr_t *)&tmp2map;
    msghdr->mq_start = msghdr->mq_end = 1;
    msghdr->mq_size = (nrmqmax*__PAGESIZE)/sizeof(msg_t);
    msghdr->mq_buffstart = msghdr->mq_buffend = msghdr->mq_buffmin = (nrmqmax+1);
    msghdr->mq_buffsize = __SLOTSIZE/__PAGESIZE/2;

#if DEBUG
    if(debug&DBG_TASKS)
        kprintf("Task %x memroot %x stack %x %s\n",tcb->pid,tcb->memroot,tcb->sp,cmdline);
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
/*
    tcb->rip = (uint64_t)&idle;
    tcb->cs = 0x8;  //ring 0 selectors
    tcb->ss = 0x10;
*/
    return tcb;
}

/**
 * map an existing address space's vmm tables
 */
tcb_t *vmm_maptask(phy_t memroot)
{
    tcb_t *tcb = (tcb_t*)&tmpmap;
    uint64_t *vmm=(uint64_t*)&tmp3map, *paging = (uint64_t *)&tmpmap;
    virt_t virt;
    vmm_map((uint64_t)&tmpmap, memroot, PG_CORE_NOCACHE|PG_PAGE);
    vmm_map((uint64_t)&tmpmap, paging[0], PG_CORE_NOCACHE|PG_PAGE);
    vmm_map((uint64_t)&tmp3map, paging[0], PG_CORE_NOCACHE|PG_PAGE);
    vmm_map((uint64_t)&tmpmap, paging[0], PG_CORE_NOCACHE|PG_PAGE);
    vmm_map(VMM_ADDRESS, paging[0], PG_CORE_NOCACHE|PG_PAGE);
    vmm_map((uint64_t)&tmpmap, paging[0], PG_CORE_NOCACHE|PG_PAGE);
    paging=(uint64_t*)VMM_ADDRESS;
    lastpt=511;
    do {
        lastpt++;
        if(vmm[lastpt/512]==0) {
            vmm[lastpt/512]=(uint64_t)pmm_alloc(1)|PG_CORE_NOCACHE|PG_PAGE;
            virt=VMM_ADDRESS+(lastpt/512)*__PAGESIZE;
            asm volatile ("dsb ishst; tlbi vaae1, %0; dc cvau, %1; dsb ish; isb" : : "r" (virt>>12), "r"(virt));
            tcb->allocmem++;
        }
    } while(paging[lastpt]!=0);
    return tcb;
}

/**
 * Set up the main entry point
 */
void vmm_setexec(virt_t func)
{
    tcb_t *tcb = (tcb_t*)&tmpmap;
    if(tcb->pc==0) {
        func-=8;
        vmm_map((virt_t)&tmp2map,*((uint64_t*)vmm_getpte(func&~0xFFF)),PG_CORE|PG_PAGE);
        *((uint32_t*)(&tmp2map+(func&0xFFF)+0))=0xf84087fe; // ldr   x30, [sp], #8
        *((uint32_t*)(&tmp2map+(func&0xFFF)+4))=0xd65f03c0; // ret   x30
        tcb->pc=func;
//kprintf("setexec %x\n",tcb->pc);
    }
}

/**
 * Add and entry point to stack
 */
void vmm_pushentry(virt_t func)
{
    tcb_t *tcb = (tcb_t*)&tmpmap;
    tcb->sp-=8;
    *((uint64_t*)tcb->sp) = func;
//kprintf("pushentry %x\n",func);
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
    virt_t virt;
    if(vmm[lastpt/512]==0) {
        vmm[lastpt/512]=(uint64_t)pmm_alloc(1)|PG_CORE_NOCACHE|PG_PAGE;
        virt=VMM_ADDRESS+(lastpt/512)*__PAGESIZE;
        asm volatile ("dsb ishst; tlbi vaae1, %0; dc cvau, %1; dsb ish; isb" : : "r" (virt>>12), "r"(virt));
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
    virt_t virt;
#if DEBUG
//    if(debug&DBG_VMM)
        kprintf("vmm_mapbuf @%d %x %d\n",lastpt, buf,npages);
#endif
    for(i=0;i<npages;i++) {
        vmm_checklastpt();
        virt=(uint64_t)buf+i*__PAGESIZE;
        asm volatile ("dsb ishst; tlbi vaae1, %0; dc cvau, %1; dsb ish; isb" : : "r" (virt>>12), "r"(virt));
        paging[lastpt++]=((*((uint64_t*)vmm_getpte(virt)))&~(__PAGESIZE-1))|
            PG_PAGE|access|(access&(1<<7)?0:1UL<<PG_NX_BIT);
        tcb->linkmem++;
    }
    return (lastpt-npages)*__PAGESIZE;
}

/**
 * map another task's message queue into this virtual address space
 */
void vmm_mq(phy_t tcbmemroot)
{
}

/**
 * map a buffer from another task's address space into this address space
 */
void vmm_buf(phy_t tcbmemroot, virt_t ptr, size_t size)
{
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
    phys &= ~((0xFFFFUL<<48)|(__PAGESIZE-1));
    access &= (__PAGESIZE-1+(1UL<<PG_NX_BIT));
    if(access==0)
        access=PG_USER_RW;
    // mark writable pages no execute too
    if(!(access & (1<<7)))
        access |= (1UL<<PG_NX_BIT);
#if DEBUG
//    if(debug&DBG_VMM)
        kprintf("    vmm_mapbss(%x,%x,%x,%d,%x)\n", tcb->memroot, bss, phys, size, access);
#endif
again:
    vmm_map((uint64_t)&tmp3map, tcb->memroot, PG_CORE_NOCACHE|PG_PAGE);
    //L1
    i=(bss>>(12+9+9))&0x1FF;
    if((paging[i]&(__PAGESIZE-1))==0) {
        paging[i]=(uint64_t)pmm_alloc(1) | PG_USER_RW | PG_PAGE;
        asm volatile("dsb sy;isb");
        tcb->allocmem++;
    }
    vmm_map((uint64_t)&tmp3map, paging[i], PG_CORE_NOCACHE|PG_PAGE);
    //L2
    i=(bss>>(12+9))&0x1FF;
    if((paging[i]&(__PAGESIZE-1))==0) {
        //map 2M at once if it's aligned and big
        if(!(phys&(__SLOTSIZE-1)) && size>=__SLOTSIZE) {
            //fill PD records
            size=((size+__SLOTSIZE-1)/__SLOTSIZE)*__SLOTSIZE;
            while(size>0) {
                paging[i] = (uint64_t)phys | access | PG_SLOT | (1UL<<PG_NX_BIT);
                asm volatile ("dsb ishst; tlbi vaae1, %0; dc cvau, %1; dsb ish; isb" : : "r" (bss>>12), "r"(bss));
                bss  += __SLOTSIZE;
                phys += __SLOTSIZE;
                size -= __SLOTSIZE;
                tcb->linkmem += __SLOTSIZE/__PAGESIZE;
                i++; if(i>511) goto again;
            }
            return;
        }
        paging[i]=(uint64_t)pmm_alloc(1) | PG_USER_RW | PG_PAGE;
        asm volatile("dsb sy;isb");
        tcb->allocmem++;
    }
    vmm_map((uint64_t)&tmp3map, paging[i], PG_CORE_NOCACHE|PG_PAGE);
    //L3
    i=(bss>>(12))&0x1FF;
    //fill L3 records
    size=((size+__PAGESIZE-1)/__PAGESIZE)*__PAGESIZE;
    while(size>0) {
        paging[i] = (uint64_t)phys | access | PG_PAGE | (1UL<<PG_NX_BIT);
        asm volatile ("dsb ishst; tlbi vaae1, %0; dc cvau, %1; dsb ish; isb" : : "r" (bss>>12), "r"(bss));
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
    vmm_map((uint64_t)&tmpmap, tcb->memroot, PG_CORE_NOCACHE|PG_PAGE);
    //L1
    i=(bss>>(12+9+9))&0x1FF;
    vmm_map((uint64_t)&tmpmap, paging[i], PG_CORE_NOCACHE|PG_PAGE);
    //L2
    i=(bss>>(12+9))&0x1FF;
    if(paging[i]&PG_SLOT) {
        while(size>0) {
            if(paging[i]&1){
                pmm_free((phy_t)(paging[i]&~(__PAGESIZE-1+(1UL<<63))), __SLOTSIZE/__PAGESIZE);
                paging[i] = 0;
                tcb->linkmem -= __SLOTSIZE/__PAGESIZE;
                asm volatile ("dsb ishst; tlbi vaae1, %0; dc cvau, %1; dsb ish; isb" : : "r" (bss>>12), "r"(bss));
            }
            bss  += __SLOTSIZE;
            size -= __SLOTSIZE;
            i++; if(i>511) goto again;
        }
    }
    //L3
    vmm_map((uint64_t)&tmpmap, paging[i], PG_CORE_NOCACHE|PG_PAGE);
    i=(bss>>(12))&0x1FF;
    while(size>0) {
        if(paging[i]&1) {
            pmm_free((phy_t)(paging[i]&~(__PAGESIZE-1+(1UL<<63))), 1);
            paging[i] = 0;
            tcb->linkmem--;
            asm volatile ("dsb ishst; tlbi vaae1, %0; dc cvau, %1; dsb ish; isb" : : "r" (bss>>12), "r"(bss));
        }
        bss  += __PAGESIZE;
        size -= __PAGESIZE;
        i++; if(i>511) goto again;
    }

    //free mapping pages
    vmm_map((uint64_t)&tmpmap, tcb->memroot, PG_CORE_NOCACHE|PG_PAGE);
    //L1
    for(i=0;i<512;i++) {
        if(paging[i]&1) {
            //L2
            vmm_map((uint64_t)&tmp2map, paging[i], PG_CORE_NOCACHE|PG_PAGE);
            sj=0;
            for(j=0;j<512;j++) {
                if(paging2[j]&1 && (paging2[j]&PG_SLOT)==0) {
                    //L3
                    vmm_map((uint64_t)&tmp3map, paging2[j], PG_CORE_NOCACHE|PG_PAGE);
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
