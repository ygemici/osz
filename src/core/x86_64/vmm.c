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

dataseg phy_t pdpe;

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
 * map memory into task's address space
 */
void vmm_mapbss(OSZ_tcb *tcb, virt_t bss, phy_t phys, size_t size, uint64_t access)
{
    if(size==0)
        return;
    uint64_t *paging = (uint64_t *)&tmpmap;
    int i;
    phys &= ~(__PAGESIZE-1);
    access &= (__PAGESIZE-1+(1UL<<63));
    if(access==0)
        access=PG_USER_RW;
    // mark writable pages no execute too
    if(access & 2)
        access |= (1UL<<63);
#if DEBUG
    if(debug&DBG_VMM)
        kprintf("    vmm_mapbss(%x,%x,%x,%d,%x)\n", tcb->memroot, bss, phys, size, access);
#endif
again:
    kmap((uint64_t)&tmpmap, tcb->memroot, PG_CORE_NOCACHE);
    //PML4E
    i=(bss>>(12+9+9+9))&0x1FF;
    if((paging[i]&~(__PAGESIZE-1))==0) {
        paging[i]=(uint64_t)pmm_alloc(1) + access;
        tcb->allocmem++;
    }
    pdpe=(phy_t)(paging[i] & ~(__PAGESIZE-1));
    kmap((uint64_t)&tmpmap, paging[i], PG_CORE_NOCACHE);
    //PDPE
    i=(bss>>(12+9+9))&0x1FF;
    pdpe+=i*8;
    if((paging[i]&~(__PAGESIZE-1))==0) {
        paging[i]=(uint64_t)pmm_alloc(1) + access;
        tcb->allocmem++;
    }
    kmap((uint64_t)&tmpmap, paging[i], PG_CORE_NOCACHE);
    //PDE
    i=(bss>>(12+9))&0x1FF;
    if((paging[i]&~(__PAGESIZE-1))==0) {
        //map 2M at once if it's aligned and big
        if(phys&~((1<<(12+9))-1) && size>=__SLOTSIZE) {
            //fill PD records
            size=((size+__SLOTSIZE-1)/__SLOTSIZE)*__SLOTSIZE;
            while(size>0) {
                paging[i] = (uint64_t)phys + (access | PG_SLOT);
                __asm__ __volatile__ ( "invlpg (%0)" : : "b"(bss) : "memory" );
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
    kmap((uint64_t)&tmpmap, paging[i], PG_CORE_NOCACHE);
    //PTE
    i=(bss>>(12))&0x1FF;
    //fill PT records
    size=((size+__PAGESIZE-1)/__PAGESIZE)*__PAGESIZE;
    while(size>0) {
        paging[i] = (uint64_t)phys + access;
        __asm__ __volatile__ ( "invlpg (%0)" : : "b"(bss) : "memory" );
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
void vmm_unmapbss(OSZ_tcb *tcb, virt_t bss, size_t size)
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
    kmap((uint64_t)&tmpmap, tcb->memroot, PG_CORE_NOCACHE);
    //PML4E
    i=(bss>>(12+9+9+9))&0x1FF;
    kmap((uint64_t)&tmpmap, paging[i], PG_CORE_NOCACHE);
    //PDPE
    i=(bss>>(12+9+9))&0x1FF;
    kmap((uint64_t)&tmpmap, paging[i], PG_CORE_NOCACHE);
    //PDE
    i=(bss>>(12+9))&0x1FF;
    if(paging[i]&PG_SLOT) {
        while(size>0) {
            if(paging[i]&1){
                pmm_free((phy_t)(paging[i]&~(__PAGESIZE-1+(1UL<<63))), __SLOTSIZE/__PAGESIZE);
                paging[i] = 0;
                tcb->linkmem -= __SLOTSIZE/__PAGESIZE;
                __asm__ __volatile__ ( "invlpg (%0)" : : "b"(bss) : "memory" );
            }
            bss  += __SLOTSIZE;
            size -= __SLOTSIZE;
            i++; if(i>511) goto again;
        }
    }
    //PTE
    kmap((uint64_t)&tmpmap, paging[i], PG_CORE_NOCACHE);
    i=(bss>>(12))&0x1FF;
    while(size>0) {
        if(paging[i]&1) {
            pmm_free((phy_t)(paging[i]&~(__PAGESIZE-1+(1UL<<63))), 1);
            paging[i] = 0;
            tcb->linkmem--;
            __asm__ __volatile__ ( "invlpg (%0)" : : "b"(bss) : "memory" );
        }
        bss  += __PAGESIZE;
        size -= __PAGESIZE;
        i++; if(i>511) goto again;
    }

    //free mapping pages
    kmap((uint64_t)&tmpmap, tcb->memroot, PG_CORE_NOCACHE);
    //PML4E
    i=(bss>>(12+9+9+9))&0x1FF;
    kmap((uint64_t)&tmpmap, paging[i], PG_CORE_NOCACHE);
    //PDPE
    for(i=0;i<512;i++) {
        if(paging[i]&1) {
            //PDE
            kmap((uint64_t)&tmp2map, paging[i], PG_CORE_NOCACHE);
            sj=0;
            for(j=0;j<512;j++) {
                if(paging2[j]&1 && (paging2[j]&PG_SLOT)==0) {
                    //PTE
                    kmap((uint64_t)&tmp3map, paging2[j], PG_CORE_NOCACHE);
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
