/*
 * core/pmm.c
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
 * @brief Physical Memory Manager architecture specific part
 */

#include <arch.h>

/* get addresses from linker script */
extern uint8_t pmm_entries;
/* memory pointers to allocate */
extern rela_t *relas;
/* syslog buffer */
extern char *syslog_buf;
extern char *syslog_ptr;
/* pointer to tmpmap in PT */
extern uint64_t *vmm_tmp;
extern uint64_t locks;
extern char *loadedelf;

/* Main scructure */
pmm_t pmm;

#if DEBUG
void pmm_dump()
{
    int i;
    pmm_entry_t *fmem = pmm.entries;
    kprintf("pmm size: %d, free: %d, total: %d ", pmm.size, pmm.freepages, pmm.totalpages);
    kprintf("bss: %x - %x\n",pmm.bss,pmm.bss_end);
    for(i=0;i<pmm.size;i++) {
        kprintf("  %x %9d\n", fmem->base, fmem->size);
        fmem++;
    }
}
#endif

/**
 * allocate continous physical memory
 * returns physical address
 */
void* pmm_alloc(int pages)
{
    pmm_entry_t *fmem = pmm.entries;
    int i=0;
    uint64_t b=0;
    klockacquire(LOCK_PMM);
    // run-time asserts
    if(pmm.freepages>pmm.totalpages)
        kprintf("WARNING shound not happen pmm.freepages %d > pmm.totalpages %d",pmm.freepages,pmm.totalpages);
    while(i<pmm.size) {
        if(fmem->size>=pages) {
            /* add entropy */
            srand[(i+0)%4] ^= (uint64_t)fmem->base/__PAGESIZE;
            srand[(i+2)%4] ^= (uint64_t)fmem->base;
            kentropy();
            /* allocate page */
            b=fmem->base;
            fmem->base+=pages*__PAGESIZE;
            fmem->size-=pages;
            if(fmem->size==0) {
                for(;i<pmm.size-1;i++) {
                    pmm.entries[i].base = pmm.entries[i+1].base;
                    pmm.entries[i].size = pmm.entries[i+1].size;
                }
                pmm.size--;
            }
            break;
        }
        fmem++; i++;
    }
    klockrelease(LOCK_PMM);
    if(b!=0) {
        pmm.freepages-=pages;
        for(i=0;i<pages;i++) {
            vmm_map((virt_t)&tmp2map, b + i*__PAGESIZE, PG_CORE_NOCACHE|PG_PAGE);
            kmemset((char *)&tmp2map, 0, __PAGESIZE);
        }
#if DEBUG
        if(debug&DBG_PMM)
            kprintf("pmm_alloc(%d)=%x pid %x\n",pages,b,
                *((uint32_t*)0) == OSZ_TCB_MAGICH?((tcb_t*)0)->pid:0);
#endif
    } else {
    // out of memory, should never happen during boot
#if DEBUG
        pmm_dump();
#endif
        kpanic("pmm_alloc: Out of physical RAM");
    }
    return (void*)b;
}

/**
 * allocate a physical page slot (2M)
 * returns physical address
 */
void* pmm_allocslot()
{
    pmm_entry_t *fmem = pmm.entries;
    int i,j;
    uint64_t p,b,s;

    klockacquire(LOCK_PMM);
    for(i=0;i<pmm.size;i++) {
        b=((fmem->base+__SLOTSIZE-1)/__SLOTSIZE)*__SLOTSIZE;
        if(fmem->base+fmem->size*__PAGESIZE>=b+__SLOTSIZE) {
            /* add entropy */
            srand[(i+0)%4] ^= fmem->base;
            srand[(i+3)%4] ^= fmem->base;
            kentropy();
            /* split the memory fragment in two */
            if(b!=fmem->base) {
                if((pmm.size+1)*sizeof(pmm_entry_t)>(uint64_t)(__PAGESIZE * nrphymax))
                    goto panic;
                for(j=pmm.size;j>i;j--) {
                    pmm.entries[j].base = pmm.entries[j-1].base;
                    pmm.entries[j].size = pmm.entries[j-1].size;
                }
                pmm.size++;
                s=(b-fmem->base)/__PAGESIZE;
                fmem->size = s;
                fmem++;
            } else {
                s=0;
            }
            /* allocate page slot */
            fmem->base = b+__SLOTSIZE;
            fmem->size -= s+(__SLOTSIZE/__PAGESIZE);
            if(fmem->size==0) {
                for(j=i;j<pmm.size-1;j++) {
                    pmm.entries[j].base = pmm.entries[j+1].base;
                    pmm.entries[j].size = pmm.entries[j+1].size;
                }
                pmm.size--;
            }
            pmm.freepages -= (__SLOTSIZE/__PAGESIZE);
            if(*((uint32_t*)0) == OSZ_TCB_MAGICH) {
                p=b;
                for(j=0;j<__SLOTSIZE/__PAGESIZE;j++) {
                    vmm_map((virt_t)&tmp2map, p, PG_CORE_NOCACHE|PG_PAGE);
                    kmemset((char *)&tmp2map, 0, __PAGESIZE);
                    p+=__PAGESIZE;
                }
            } else {
                kmemset((char *)b, 0, __SLOTSIZE);
            }
#if DEBUG
            if(debug&DBG_PMM)
                kprintf("pmm_allocslot=%x pid %x\n",b,
                    *((uint32_t*)0) == OSZ_TCB_MAGICH?((tcb_t*)0)->pid:0);
#endif
            klockrelease(LOCK_PMM);
            return (void*)b;
        }
        fmem++;
    }
panic:
    // out of memory, should never happen during boot
    //if(*((uint32_t*)0) != OSZ_TCB_MAGICH) {
#if DEBUG
        pmm_dump();
#endif
        kpanic("pmm_allocslot: Out of physical RAM");
    //}
    klockrelease(LOCK_PMM);
    return NULL;
}

/**
 * Add to free memory entries
 */
void pmm_free(phy_t base, size_t numpages)
{
    int i;
    uint64_t s=numpages*__PAGESIZE;
    klockacquire(LOCK_PMM);
    /* add entropy */
    srand[(numpages+0)%4] ^= (uint64_t)base;
    srand[(numpages+2)%4] ^= (uint64_t)base;
    kentropy();
#if DEBUG
    if(debug&DBG_PMM)
        kprintf("pmm_free(%x,%d) pid %x\n",base,numpages,((tcb_t*)0)->pid);
#endif
    /* check for contiguous regions */
    for(i=0;i<pmm.size;i++) {
        /* add to the beginning */
        if(pmm.entries[i].base == base+s)
            goto addregion;
        /* add to the end */
        if(pmm.entries[i].base+pmm.entries[i].size*__PAGESIZE == base)
            goto addregion2;
    }
    /* add as a new entry */
    pmm.size++;
    pmm.entries[i].size = 0;
addregion:
    pmm.entries[i].base = base;
addregion2:
    pmm.entries[i].size += numpages;
    pmm.freepages += numpages;
    if(pmm.freepages>pmm.totalpages)
        kprintf("WARNING shound not happen pmm.freepages %d > pmm.totalpages %d",pmm.freepages,pmm.totalpages);
    klockrelease(LOCK_PMM);
}

/**
 * initialize Physical Memory Manager
 */
void pmm_init()
{
    char *types[] = { "used", "free", "ACPI", "ANVS", "MMIO" };
    // memory map provided by the loader
    MMapEnt *entry = (MMapEnt*)&bootboot.mmap;
    // our new free pages directory, pmm.entries
    pmm_entry_t *fmem;
    uint64_t num;
    uint64_t i, m=0, n, *pte, e=bootboot.initrd_ptr+((bootboot.initrd_size+__PAGESIZE-1)/__PAGESIZE)*__PAGESIZE;

    if(kmemcmp(&bootboot.magic,BOOTBOOT_MAGIC,4) || bootboot.size>__PAGESIZE)
        kpanic("Memory map corrupt");

    // allocate at least 2 pages for free memory entries
    // that's 2*4096/16 = 512 maximum fragments
    if(nrphymax<2)
        nrphymax=2;
    // minimum for syslog buffer pages
    if(nrlogmax<1)
        nrlogmax=1;
    // initialize pmm structure
    pmm.magic = OSZ_PMM_MAGICH;
    pmm.size = 0;
    // first bss pages are for temporary mappings, tmpmap and tmp2map
    // and their control page tables. Let's initialize them
    vmm_tmp = vmm_init();

    // buffers
    pmm.entries = fmem = (pmm_entry_t*)(&pmm_entries);
    pmm.bss     = (uint8_t*)&__dynbss_start;
    pmm.bss_end = (uint8_t*)&pmm_entries + (uint64_t)(__PAGESIZE * nrphymax);
    // this is a chicken and egg scenario. We need free memory to
    // store the free memory table...
    num = (bootboot.size-128)/16;
#if DEBUG
    if(debug&DBG_MEMMAP)
        kprintf("\nMemory Map (%d entries)\n", num);
#endif
    i=false; srand[0]++; srand[1]--;
    while(num>0) {
        srand[num%4]++; srand[(num+2)%4]++;
        srand[(entry->ptr/__PAGESIZE + entry->size +
            1000*bootboot.datetime[7] + 100*bootboot.datetime[6] + bootboot.datetime[5])%4] *= 16807;
        kentropy();
        n=MMapEnt_Ptr(entry)+MMapEnt_Size(entry);
#if DEBUG
        if(debug&DBG_MEMMAP)
            kprintf("  %s %8x %8x %11d\n",
                MMapEnt_Type(entry)<5?types[MMapEnt_Type(entry)]:types[0],
                MMapEnt_Ptr(entry), n, MMapEnt_Size(entry)
            );
#endif
        if(MMapEnt_IsFree(entry)) {
            //get upper bound for free memory
            if(n > m) m = n;
            if((uint)(MMapEnt_Size(entry)/__PAGESIZE)>=(uint)nrphymax) {
                //failsafe, make it page aligned
                if(entry->ptr & (__PAGESIZE-1)) {
                    entry->size -= (__PAGESIZE-(entry->ptr & (__PAGESIZE-1))) & 0xFFFFFFFFFFFFF0ULL;
                    entry->ptr = ((entry->ptr+__PAGESIZE)/__PAGESIZE)*__PAGESIZE;
                }
                if(!i) {
                    // map free physical pages to pmm.entries. Nocache is important for MultiCore
                    for(i=0;(uint)i<(uint)nrphymax;i++)
                        vmm_map((uint64_t)((uint8_t*)&pmm_entries) + i*__PAGESIZE,
                            (uint64_t)MMapEnt_Ptr(entry) + i*__PAGESIZE,
                            PG_CORE_NOCACHE|PG_PAGE);
                    // "preallocate" the memory for pmm.entries
                    entry->ptr +=(uint64_t)(((uint)nrphymax)*__PAGESIZE);
                    entry->size-=(uint64_t)(((uint)nrphymax)*__PAGESIZE);
                    i=true;
                }
            }
        }
        num--;
        entry++;
    }
    // memory check, -1 for rounding errors
    if(m/1024/1024 < PHYMEM_MIN-1)
        kpanic("Not enough memory. At least %d Mb of RAM required, only %d Mb free.", PHYMEM_MIN, m/1024/1024);

    // set up the pmm lock
    locks = 0;

    // iterate through memory map again but this time
    // record free areas in pmm.entries
    num = (bootboot.size-128)/16;
    entry = &bootboot.mmap;
    pmm.totalpages = 0;
    while(num>0) {
        srand[(entry->ptr/__PAGESIZE+num)%4] += entry->size + 
            1000*bootboot.datetime[7] - 100*bootboot.datetime[6] + bootboot.datetime[5];
        srand[(entry->ptr/__PAGESIZE+entry->size)%4] *= 16807;
        kentropy();
        if(MMapEnt_IsFree(entry)) {
            // bootboot.initrd_ptr should already be excluded, but be sure, failsafe
            // a: +------------+    initrd at begining of area
            //    ######
            // b: +------------+    initrd at the end
            //            ######
            // c: +------------+    initrd in the middle
            //        ######
            // d: ------++------    initrd over areas boundary
            //        ######
            if((MMapEnt_Ptr(entry)<=bootboot.initrd_ptr && MMapEnt_Ptr(entry)+MMapEnt_Size(entry)>bootboot.initrd_ptr)||
               (MMapEnt_Ptr(entry)<e && MMapEnt_Ptr(entry)+MMapEnt_Size(entry)>=e)) {
                // begin
                if(bootboot.initrd_ptr>MMapEnt_Ptr(entry)+__PAGESIZE) {
                    fmem->base = MMapEnt_Ptr(entry);
                    fmem->size = (bootboot.initrd_ptr-MMapEnt_Ptr(entry))/__PAGESIZE;
                    pmm.size++;
                    pmm.totalpages += fmem->size;
                    fmem++;
                }
                // end
                if(MMapEnt_Ptr(entry)+MMapEnt_Size(entry)>e+__PAGESIZE) {
                    fmem->base = bootboot.initrd_ptr+bootboot.initrd_size;
                    fmem->size = (MMapEnt_Ptr(entry)+MMapEnt_Size(entry)-e)/__PAGESIZE;
                    pmm.size++;
                    pmm.totalpages += fmem->size;
                    fmem++;
                }
            } else {
                fmem->base = MMapEnt_Ptr(entry);
                fmem->size = MMapEnt_Size(entry)/__PAGESIZE;
                if(fmem->size!=0) {
                    pmm.size++;
                    pmm.totalpages += fmem->size;
                    fmem++;
                } else {
                    fmem->base = 0;
                }
            }
        }
        num--;
        entry++;
    }
    pmm.freepages = pmm.totalpages;
    // ok, now we can allocate bss memory for core

    // These are compile time configurable buffer sizes,
    // but that's ok, sized to the needs of this source
    relas = (rela_t*)kalloc(2);
    // allocate loaded elf list
    loadedelf = (char*)kalloc(1);
    //allocate syslog buffer
    syslog_buf = syslog_ptr = kalloc(nrlogmax);

    //first real message
    syslog_early(OSZ_NAME " " OSZ_VER " " OSZ_ARCH "-" OSZ_PLATFORM);
    //make sure we don't expose random seed by generating a boot session uuid
    i=srand[0]^srand[2];
    syslog_early("Started uuid %4x-%2x-%2x-%8x",
        (uint32_t)i,(uint16_t)(i>>32),(uint16_t)(i>>48),srand[1]^srand[3]);
    syslog_early("Frame buffer %d x %d @%x sc %d",bootboot.fb_width,bootboot.fb_height,bootboot.fb_ptr,bootboot.fb_scanline);

    //dump memory map to log
    num = (bootboot.size-128)/16;
    entry = (MMapEnt*)&bootboot.mmap;
    syslog_early("Memory Map (%d entries, %d Mb free)\n", num, pmm.totalpages*__PAGESIZE/1024/1024);
    while(num>0) {
        syslog_early(" %s %8x %11d\n",
            MMapEnt_Type(entry)<5?types[MMapEnt_Type(entry)]:types[0],
            MMapEnt_Ptr(entry), MMapEnt_Size(entry)
        );
        num--;
        entry++;
    }

    //map initrd in kernel space (up to 16M)
    m=(bootboot.initrd_size+__SLOTSIZE-1)/__SLOTSIZE;
    pte=(uint64_t*)pmm_alloc(m);
    for(i=0;i<m;i++)
        *((uint64_t*)(&tmpmqctrl + (i<<3))) = ((phy_t)pte + i*__PAGESIZE)|PG_CORE|PG_PAGE;
    e-=bootboot.initrd_ptr; e/=__PAGESIZE;
    for(i=0;i<e;i++) {
        vmm_map(INITRD_ADDRESS+i*__PAGESIZE,bootboot.initrd_ptr + i*__PAGESIZE,PG_CORE|PG_PAGE);
        kentropy();
    }
}

/**
 * allocate kernel bss
 * returns linear address
 */
void* kalloc(int pages)
{
    void *bss = pmm.bss_end;
    if(pages<1)
        pages=1;
    if(pages>512)
        pages=512;
    /* we allow non continous pages. For multi core system we must disable caching */
    while(pages-->0){
        vmm_map((uint64_t)pmm.bss_end,(uint64_t)pmm_alloc(1),PG_CORE_NOCACHE|PG_PAGE);
        pmm.bss_end += __PAGESIZE;
    }
    return bss;
}

/**
 * we never need to free kernel bss, just for completeness
 */
void kfree(void* ptr)
{
}
