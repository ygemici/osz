/*
 * pmm.c
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

#include "core.h"
#include "pmm.h"

extern void* __bss_start;

// Preallocate these as allocation relies on them
OSZ_pmm __attribute__ ((section (".data")))
	pmm;
OSZ_pmm_entry __attribute__ ((section (".data")))
	pmm_entries[__PHYFRAG_MAX];

void* kalloc(int pages)
{
	//phys=pmm_alloc()
	//pmm_map(virt,phys)
    return 0;
}

void kfree(void* ptr)
{
}

void* pmm_alloc(int pages)
{
    return 0;
}

void pmm_init()
{
	// memory map provided by the loader
	MMapEnt *entry = (MMapEnt*)&bootboot.mmap;
	// our new free pages directory
	OSZ_pmm_entry *fmem;
	uint num = bootboot.mmap_num;
	char first=1;

	// set up structures
	pmm.magic = OSZ_PMM_MAGICH;
	pmm.size = 32;
	pmm.buff = fmem = __bss_start;
	pmm.bss = pmm.bss_end = __bss_start + __PAGESIZE *
		(__PHYFRAG_MAX>2?__PHYFRAG_MAX:2);
	// iterate through memory map and record free areas
	while(num>0) {
		if(MMapEnt_IsFree(entry)) {
			if(first) {
				// chicken and the egg problem. We cannot use alloc yet
				// since the free memory directory doesn't even mapped
				kmap(__bss_start, (void*)&MMapEnt_Ptr(entry));
				// now there's one mapped free page at least for fmem,
				// but no base+length pairs saved there yet
			}
			fmem->base = MMapEnt_Ptr(entry);
			fmem->size = MMapEnt_Size(entry)/__PAGESIZE;
			if(first) {
				// now the chicken. "Allocate" one page for fmem
				// this assumes fragmentation will be below 256 entries.
				// TODO: worth mapping more pages?
				fmem->base += __PAGESIZE;
				fmem->size--;
				first=0;
			}
			if(fmem->size>0) {
				fmem++;
				pmm.size+=sizeof(MMapEnt);
			}
		}
		num--;
		entry++;
	}
}
