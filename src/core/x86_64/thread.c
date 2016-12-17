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
#include <elf.h>
#include <fsZ.h>

/* external resources */
extern uint8_t MQ_ADDRESS;
extern OSZ_pmm pmm;

uint64_t __attribute__ ((section (".data"))) core_mapping;
uint64_t __attribute__ ((section (".data"))) shlib_mapping;

typedef unsigned char *valist;
#define vastart(list, param) (list = (((valist)&param) + sizeof(void*)*8))
#define vaarg(list, type)    (*(type *)((list += sizeof(void*)) - sizeof(void*)))

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
    // PT shared libs
    ptr2=pmm_alloc();
    paging[1]=(uint64_t)ptr2+1;
    kmap((uint64_t)&tmp2map, (uint64_t)ptr2, PG_CORE_NOCACHE);
    shlib_mapping=(uint64_t)pmm_alloc();
    paging[0]=shlib_mapping+1;
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

/* load an ELF64 executable into text segment starting at 2M */
void *thread_loadelf(char *fn)
{
    OSZ_tcb *tcb = (OSZ_tcb*)(pmm.bss_end);
    uint64_t *paging = (uint64_t *)&tmp2map;
    Elf64_Ehdr *elf=(Elf64_Ehdr *)fs_locate(fn);
    Elf64_Phdr *phdr_c=(Elf64_Phdr *)((uint8_t *)elf+elf->e_phoff);
    Elf64_Phdr *phdr_d=(Elf64_Phdr *)((uint8_t *)elf+elf->e_phoff+elf->e_phentsize);
    int i=0,ret,size=(fs_size+__PAGESIZE-1)/__PAGESIZE;
    // valid elf for this platform?
    if(elf==NULL || kmemcmp(elf->e_ident,ELFMAG,SELFMAG) ||
        elf->e_ident[EI_CLASS]!=ELFCLASS64 ||
        elf->e_ident[EI_DATA]!=ELFDATA2LSB ||
        elf->e_phnum<2 ||
        // code segment, page aligned, readable, executable, but not writable
        phdr_c->p_type!=PT_LOAD || (phdr_c->p_vaddr&(__PAGESIZE-1))!=0 || (phdr_c->p_flags&PF_W)!=0 ||
        // data segment, page aligned, readable, writeable, but not executable
        phdr_d->p_type!=PT_LOAD || (phdr_d->p_vaddr&(__PAGESIZE-1))!=0 || (phdr_d->p_flags&PF_X)!=0
        )
        return NULL;
    // PT at tmp2map
    while((paging[i]&1)!=0 && i<1024-size) i++;
    if((paging[i]&1)!=0) {
        kpanic("thread_loadelf: Out of memory");
    }
    ret = i;
#if DEBUG
    kprintf("  loadelf %s %x @%d ",fn,elf,ret);
    kprintf("code:%x (%d) data:%x (%d)\n",
        phdr_c->p_paddr, phdr_c->p_filesz,
        phdr_d->p_paddr, phdr_d->p_filesz
        );
#endif
    /* map text segment */
    size=(phdr_c->p_filesz+__PAGESIZE-1)/__PAGESIZE;
    while(size--) {
        paging[i]=(uint64_t)((char*)elf + (i-ret)*__PAGESIZE + PG_USER_RO);
        tcb->linkmem++;
        i++;
    }
    /* allocate data segment */
    size=(phdr_d->p_filesz+__PAGESIZE-1)/__PAGESIZE;
    while(size--) {
        void *pm = pmm_alloc();
        kmemcpy(pm,(char *)elf + (i-ret)*__PAGESIZE,phdr_d->p_filesz);
        paging[i]=(uint64_t)(pm) + PG_USER_RW;
        tcb->allocmem++;
        i++;
    }
    return (void*)((uint64_t)ret * __PAGESIZE);
}

/* load an ELF64 shared object into text segment at 4G */
void thread_loadso(char *fn)
{
#if DEBUG
    kprintf("  ");
#endif
    thread_loadelf(fn);
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

/* fill in .rela.plt entries. Relies on identity mapping */
void thread_dynlink(pid_t thread)
{
//    OSZ_tcb *tcb = (OSZ_tcb*)(thread*__PAGESIZE);
//    kmap((uint64_t)&tmp2map, (uint64_t)(thread*__PAGESIZE), PG_CORE_NOCACHE);
}

void thread_mapbss(uint64_t phys, uint64_t size)
{
}

/* Function to start a system service */
void service_init2(char *fn, uint64_t n, ...)
{
    valist args;
    vastart(args, n);
    pid_t pid = thread_new();

    // map executable
    thread_loadelf(fn);

    // map libc
    thread_loadso("lib/libc.so");
    // map additional shared libraries
    while(n>0) {
        thread_loadso(vaarg(args, char*));
        n--;
    }
    // dynamic linker
    thread_dynlink(pid);

    // add to queue so that scheduler will know about this thread
    thread_add(pid);
}

void service_init(char *fn)
{
    service_init2(fn, 0);
}
