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

//phy_t __attribute__ ((section (".data"))) pt;
phy_t __attribute__ ((section (".data"))) pdpe;

void exc14pagefault(uint64_t excno, uint64_t rip, uint64_t rsp)
{
//#if DEBUG
//    dbg_enable(rip,rsp,"page fault");
//#else
    kpanic(" --- page fault %d ---",excno);
//#endif
}

/* map memory into thread's address space */
void vmm_mapbss(OSZ_tcb *tcb, virt_t bss, phy_t phys, size_t size, uint64_t access)
{
    if(size==0)
        return;
    uint64_t *paging = (uint64_t *)&tmpmap;
    int i;
    kmap((uint64_t)&tmpmap, tcb->memroot, PG_CORE_NOCACHE);
    phys &= ~(__PAGESIZE-1);
    if(access==0 || access>__PAGESIZE)
        access=PG_USER_RW;
    //PML4E
    i=(bss>>(12+9+9+9))&0x1FF;
    if((paging[i]&~(__PAGESIZE-1))==0) {
        paging[i]=(uint64_t)pmm_alloc() + access;
        tcb->allocmem++;
    }
    pdpe=(phy_t)(paging[i] & ~(__PAGESIZE-1));
    kmap((uint64_t)&tmpmap, paging[i], PG_CORE_NOCACHE);
    //PDPE
    i=(bss>>(12+9+9))&0x1FF;
    pdpe+=i*8;
    if((paging[i]&~(__PAGESIZE-1))==0) {
        paging[i]=(uint64_t)pmm_alloc() + access;
        tcb->allocmem++;
    }
    kmap((uint64_t)&tmpmap, paging[i], PG_CORE_NOCACHE);
    //PDE
    i=(bss>>(12+9))&0x1FF;
    if((paging[i]&~(__PAGESIZE-1))==0) {
        //map 2M at once if it's aligned and big
        if(phys&~((1<<(12+9))-1) && size>__SLOTSIZE/2) {
            uint64_t j;
            //fill PD records
            for(j=0;j<(size+__SLOTSIZE-1)/__SLOTSIZE;j++) {
                paging[i] = (uint64_t)phys + (access | PG_SLOT);
                phys += __SLOTSIZE;
                tcb->linkmem += __SLOTSIZE/__PAGESIZE;
                i++;
            }
            return;
        }
        paging[i]=(uint64_t)pmm_alloc() + access;
        tcb->allocmem++;
    }
    kmap((uint64_t)&tmpmap, paging[i], PG_CORE_NOCACHE);
    //PTE
    i=(bss>>(12))&0x1FF;
    //fill PT records
    for(i=0;i<(size+__PAGESIZE-1)/__PAGESIZE;i++) {
        paging[i] = (uint64_t)phys + access;
        phys += __PAGESIZE;
        tcb->linkmem++;
    }
}
