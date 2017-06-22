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

#include "env.h"

/* get addresses from linker script */
extern uint8_t pmm_entries;
/* kernel variables */
extern uint64_t isr_entropy[4];
/* memory pointers to allocate */
extern OSZ_rela *relas;
extern uint64_t *safestack;
extern char *syslog_buf;
extern char *syslog_ptr;

/* Main scructure */
OSZ_pmm __attribute__ ((section (".data"))) pmm;
/* pointer to tmpmap in PT */
void __attribute__ ((section (".data"))) *kmap_tmp;

#if DEBUG
void pmm_dump()
{
    int i;
    OSZ_pmm_entry *fmem = pmm.entries;
    kprintf("pmm size: %d, free: %d, total: %d ", pmm.size, pmm.freepages, pmm.totalpages);
    kprintf("bss: %x - %x\n",pmm.bss,pmm.bss_end);
    for(i=0;i<pmm.size;i++) {
        kprintf("  %x %9d\n", fmem->base, fmem->size);
        fmem++;
    }
}
#endif

/**
 * allocate a physical page
 * returns physical address
 */
void* pmm_alloc()
{
    OSZ_pmm_entry *fmem = pmm.entries;
    int i;
    // run-time asserts
    if(pmm.freepages>pmm.totalpages)
        kprintf("WARNING shound not happen pmm.freepages %d > pmm.totalpages %d",pmm.freepages,pmm.totalpages);
    for(i=0;i<pmm.size;i++) {
        if(fmem->size>0) {
            /* add entropy */
            srand[(i+0)%4] ^= (uint64_t)fmem->base;
            srand[(i+2)%4] ^= (uint64_t)fmem->base;
            kentropy();
            /* allocate page */
            if(*((uint32_t*)0) == OSZ_TCB_MAGICH) {
                kmap((virt_t)&tmp2map, fmem->base, PG_CORE_NOCACHE);
                kmemset((char *)&tmp2map, 0, __PAGESIZE);
            } else {
                kmemset((char *)fmem->base, 0, __PAGESIZE);
            }
            fmem->base+=__PAGESIZE;
            fmem->size--;
            if(fmem->size==0) {
                for(;i<pmm.size-1;i++) {
                    pmm.entries[i].base = pmm.entries[i+1].base;
                    pmm.entries[i].size = pmm.entries[i+1].size;
                }
                pmm.size--;
            }
            pmm.freepages--;
            return (void*)(fmem->base - __PAGESIZE);
        }
        fmem++;
    }
    // out of memory, should never happen during boot
    //if(*((uint32_t*)0) != OSZ_TCB_MAGICH)
        kpanic("pmm_alloc: Out of physical RAM");
    return NULL;
}

/**
 * allocate a physical page slot (2M)
 * returns physical address
 */
void* pmm_allocslot()
{
    OSZ_pmm_entry *fmem = pmm.entries;
    int i,j;
    uint64_t p,b,s;

    for(i=0;i<pmm.size;i++) {
        b=((fmem->base+__SLOTSIZE-1)/__SLOTSIZE)*__SLOTSIZE;
        if(fmem->base+fmem->size*__PAGESIZE>=b+__SLOTSIZE) {
            /* add entropy */
            srand[(i+0)%4] ^= fmem->base;
            srand[(i+3)%4] ^= fmem->base;
            kentropy();
            /* split the memory fragment in two */
            if(b!=fmem->base) {
                if((pmm.size+1)*sizeof(OSZ_pmm_entry)>(uint64_t)(__PAGESIZE * nrphymax))
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
                    kmap((virt_t)&tmp2map, p, PG_CORE_NOCACHE);
                    kmemset((char *)&tmp2map, 0, __PAGESIZE);
                    p+=__PAGESIZE;
                }
            } else {
                kmemset((char *)b, 0, __SLOTSIZE);
            }
            return (void*)b;
        }
        fmem++;
    }
panic:
    // out of memory, should never happen during boot
    //if(*((uint32_t*)0) != OSZ_TCB_MAGICH)
        kpanic("pmm_allocslot: Out of physical RAM");
    return NULL;
}

/**
 * Add to free memory entries
 */
void pmm_free(phy_t base, size_t numpages)
{
    int i;
    uint64_t s=numpages*__PAGESIZE;
    /* add entropy */
    srand[(numpages+0)%4] ^= (uint64_t)base;
    srand[(numpages+2)%4] ^= (uint64_t)base;
    kentropy();
    /* check for continuous regions */
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
}

/**
 * initialize Physical Memory Manager
 */
void pmm_init()
{
    char *types[] = { "????", "free", "resv", "ACPI", "ANVS", "used", "MMIO" };

    // memory map provided by the loader
    MMapEnt *entry = (MMapEnt*)&bootboot.mmap;
    // our new free pages directory, pmm.entries
    OSZ_pmm_entry *fmem;
    uint num = (bootboot.size-128)/16;
    uint i;
    uint64_t m = 0, e=bootboot.initrd_ptr+((bootboot.initrd_size+__PAGESIZE-1)/__PAGESIZE)*__PAGESIZE;
    if(kmemcmp(&bootboot.magic,BOOTBOOT_MAGIC,4) || num>504)
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
    // first 3 pages are for temporary mappings, tmpmap and tmp2map
    // and their pte. Let's initialize them
    kmap_tmp = kmap_init();

    // buffers
    pmm.entries = fmem = (OSZ_pmm_entry*)(&pmm_entries);
    pmm.bss     = (uint8_t*)&__bss_start;
    pmm.bss_end = (uint8_t*)&pmm_entries + (uint64_t)(__PAGESIZE * nrphymax);
    // this is a chicken and egg scenario. We need free memory to
    // store the free memory table...
    while(num>0) {
        if(MMapEnt_IsFree(entry) &&
           (uint)(MMapEnt_Size(entry)/__PAGESIZE)>=(uint)nrphymax) {
            // map free physical pages to pmm.entries
            for(i=0;(uint)i<(uint)nrphymax;i++)
                kmap((uint64_t)((uint8_t*)fmem) + i*__PAGESIZE,
                    (uint64_t)MMapEnt_Ptr(entry) + i*__PAGESIZE,
                    PG_CORE_NOCACHE);
            // "preallocate" the memory for pmm.entries
            entry->ptr +=(uint64_t)((uint)nrphymax*__PAGESIZE);
            entry->size-=(uint64_t)((uint)nrphymax*__PAGESIZE);
            break;
        }
        num--;
        entry++;
    }

    // iterate through memory map again but this time
    // record free areas in pmm.entries
    num = (bootboot.size-128)/16;
    entry = &bootboot.mmap;
    pmm.totalpages = 0;
    if(debug&DBG_MEMMAP)
        kprintf("\nMemory Map (%d entries)\n", num);
    while(num>0) {
        if(debug&DBG_MEMMAP)
            kprintf("  %s %8x %9d\n",
                MMapEnt_Type(entry)<7?types[MMapEnt_Type(entry)]:types[0],
                MMapEnt_Ptr(entry), MMapEnt_Size(entry)
            );
        if(MMapEnt_IsFree(entry)) {
            if(MMapEnt_Ptr(entry)+MMapEnt_Size(entry) > m)
                m = MMapEnt_Ptr(entry)+MMapEnt_Size(entry);
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
                if(bootboot.initrd_ptr-MMapEnt_Ptr(entry)>__PAGESIZE) {
                    fmem->base = MMapEnt_Ptr(entry);
                    fmem->size = (bootboot.initrd_ptr-MMapEnt_Ptr(entry))/__PAGESIZE;
                    pmm.totalpages += fmem->size;
                    fmem++;
                }
                // end
                if(MMapEnt_Ptr(entry)+MMapEnt_Size(entry)-e>__PAGESIZE) {
                    fmem->base = bootboot.initrd_ptr+bootboot.initrd_size;
                    fmem->size = (MMapEnt_Ptr(entry)+MMapEnt_Size(entry)-e)/__PAGESIZE;
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
    // memory check, -1 for rounding errors
    if(m/1024/1024 < PHYMEM_MIN-1)
        kpanic("Not enough memory. At least %d Mb of RAM required, only %d Mb free.", PHYMEM_MIN, m/1024/1024);

    // ok, now we can allocate bss memory for core

    // These are compile time configurable buffer sizes,
    // but that's ok, sized to the needs of this source
    relas = (OSZ_rela*)kalloc(2);
    //allocate stack for ISRs and syscall
    safestack = kalloc(1);
    //allocate syslog buffer
    syslog_buf = syslog_ptr = kalloc(nrlogmax);

    //first real message
    syslog_early("Started uuid %4x-%2x-%2x-%8x",
        (uint32_t)srand[0],(uint16_t)srand[1],(uint16_t)srand[2],srand[3]);
    syslog_early("Frame buffer %d x %d @%x",bootboot.fb_width,bootboot.fb_height,bootboot.fb_ptr);

    //dump memory map to log
    num = (bootboot.size-128)/16;
    entry = (MMapEnt*)&bootboot.mmap;
    syslog_early("Memory Map (%d entries, %d Mb free)\n", num, pmm.totalpages*__PAGESIZE/1024/1024);
    while(num>0) {
        syslog_early(" %s %8x %9d\n",
            MMapEnt_Type(entry)<7?types[MMapEnt_Type(entry)]:types[0],
            MMapEnt_Ptr(entry), MMapEnt_Size(entry)
        );
        num--;
        entry++;
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
    while(pages-->0){
        kmap((uint64_t)pmm.bss_end,(uint64_t)pmm_alloc(),PG_CORE);
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
