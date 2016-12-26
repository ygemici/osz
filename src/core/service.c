/*
 * core/service.c
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
 * @brief System service loader
 */

#include <elf.h>
#include "env.h"

/* external resources */
extern uint64_t *stack_ptr;
extern char *drvnames;
extern uint64_t *drivers;
extern uint64_t *drvptr;
extern uint64_t isr_entropy[4];

extern unsigned char *env_dec(unsigned char *s, uint *v, uint min, uint max);

typedef unsigned char *valist;
#define vastart(list, param) (list = (((valist)&param) + sizeof(void*)*8))
#define vaarg(list, type)    (*(type *)((list += sizeof(void*)) - sizeof(void*)))

/* we define the addresses as 32 bit, as code segment cannot grow
   above 4G. This way we can have 512 relocation records in a page */

/* memory allocated for relocation addresses */
OSZ_rela __attribute__ ((section (".data"))) *relas;

/* pids of services. Negative pids are service ids and looked up in this */
pid_t  __attribute__ ((section (".data"))) services[NRSRV_MAX];
uint64_t __attribute__ ((section (".data"))) nrservices = -SRV_usrfirst;

uint64_t __attribute__ ((section (".data"))) *irq_routing_table;

/* register a user mode service for pid translation */
uint64_t service_register(pid_t thread)
{
    services[nrservices++] = thread;
    return -(nrservices-1);
}

/* load an ELF64 binary into text segment starting at 2M */
void *service_loadelf(char *fn)
{
    // failsafe
    if(fn==NULL)
        return NULL;
    // this is so early, we don't have initrd in fs task' bss yet.
    // so we have to rely on identity mapping to locate the files
    OSZ_tcb *tcb = (OSZ_tcb*)(pmm.bss_end);
    uint64_t *paging = (uint64_t *)&tmpmap;
    char *name = fn;
    // if it's in core stack, we have to copy it to a permanent store
    if(drvptr!=NULL && drvnames!=NULL && (uint64_t)fn > (uint64_t)&__bss_start) {
        name = drvnames;
        drvnames = kstrcpy(drvnames, fn);
    }
    /* locate elf on initrd */
    Elf64_Ehdr *elf=(Elf64_Ehdr *)fs_locate(fn);
    /* get program headers */
    Elf64_Phdr *phdr_c=(Elf64_Phdr *)((uint8_t *)elf+elf->e_phoff);
    Elf64_Phdr *phdr_d=(Elf64_Phdr *)((uint8_t *)elf+elf->e_phoff+elf->e_phentsize);
    Elf64_Phdr *phdr_l=(Elf64_Phdr *)((uint8_t *)elf+elf->e_phoff+2*elf->e_phentsize);
    int i=0,ret,size=(fs_size+__PAGESIZE-1)/__PAGESIZE;
    // PT at tmpmap
    while((paging[i]&1)!=0 && i<1024-size) i++;
    if((paging[i]&1)!=0) {
        kpanic("out of memory, text segment too big: %s", fn);
    }
    ret = i;
    isr_entropy[(i+1)%4] ^= (uint64_t)elf;
    isr_entropy[(i+3)%4] ^= (uint64_t)elf;
    isr_gainentropy();

#if DEBUG
    if(debug==DBG_ELF)
        kprintf("  loadelf %s %x (%d pages) @%d\n",fn,elf,size,ret);
#endif
    // valid elf for this platform?
    if(elf==NULL || kmemcmp(elf->e_ident,ELFMAG,SELFMAG) ||
        elf->e_ident[EI_CLASS]!=ELFCLASS64 ||
        elf->e_ident[EI_DATA]!=ELFDATA2LSB ||
        elf->e_phnum<3 ||
        // code segment, page aligned, readable, executable, but not writable
        phdr_c->p_type!=PT_LOAD || (phdr_c->p_vaddr&(__PAGESIZE-1))!=0 || (phdr_c->p_flags&PF_W)!=0 ||
        // data segment, page aligned, readable, writeable, but not executable
        phdr_d->p_type!=PT_LOAD || (phdr_d->p_vaddr&(__PAGESIZE-1))!=0 || (phdr_d->p_flags&PF_X)!=0 ||
        // linkage information
        phdr_l->p_type!=PT_DYNAMIC
        ) {
#if DEBUG
            kprintf("WARNING corrupt ELF binary: %s\n", fn);
#endif
            return (void*)(-1);
    }
    /* map text segment */
    size=(phdr_c->p_filesz+__PAGESIZE-1)/__PAGESIZE;
    while(size--) {
        paging[i]=(uint64_t)((char*)elf + (i-ret)*__PAGESIZE + PG_USER_RO);
        fs_size -= __PAGESIZE;
        tcb->linkmem++;
        if(drvptr!=NULL)
            drvptr[i] = (uint64_t)name;
        i++;
    }
    // failsafe
    if(fs_size<0) fs_size=0;

    /* allocate data segment, page aligned */
    size=(fs_size+__PAGESIZE-1)/__PAGESIZE;
    while(size--) {
        void *pm = pmm_alloc();
        kmemcpy(pm,(char *)elf + (i-ret)*__PAGESIZE,__PAGESIZE);
        paging[i]=((uint64_t)(pm) + PG_USER_RW)|((uint64_t)1<<63);
        tcb->allocmem++;
        if(drvptr!=NULL)
            drvptr[i] = (uint64_t)name;
        i++;
    }
    /* return start offset within text segment */
    return (void*)((uint64_t)ret * __PAGESIZE);
}

/* load an ELF64 shared library */
__inline__ void service_loadso(char *fn)
{
#if DEBUG
    if(debug==DBG_ELF)
        kprintf("  ");
#endif
    service_loadelf(fn);
}

void service_installirq(uint8_t irq, uint64_t offs)
{
    uint l,k=((irq-1)*nrirqmax)+1;
    uint last=irq*nrirqmax;
    if(irq>0 && irq<128 && k<ISR_NUMIRQ*nrirqmax-1) {
        l = k;
        // find next free slot
        while(irq_routing_table[k]!=0&&k<last) k++;
#if DEBUG
        if(debug==DBG_IRQ)
            kprintf("  IRQ #%d: irt[%d]=%x %s\n",
                irq, k, offs, drivers[(offs-TEXT_ADDRESS)/__PAGESIZE]);
        if(irq_routing_table[k]!=0)
            kprintf("WARNING too many IRQ handlers for %d\n", irq);
#endif
        irq_routing_table[k] = offs;
        // first of it's kind?
        // we won't enable IRQ2 (cascade) for real. We'll use it as
        // a video card fake irq to blit composed buffer to framebuffer.
        if (l == k && irq!=2) {
#if DEBUG
            if(debug==DBG_IRQ)
                kprintf("           unmask IRQ #%d\n", irq);
#endif
            isr_enableirq(irq);
        }
#if DEBUG
    } else if(debug==DBG_IRQ) {
            kprintf("WARNING irq_routing_table[%d] for %x out of range\n", k, offs);
#endif
    }
}

/* Fill in GOT entries. Relies on identity mapping
 *  - tmpmap: the text segment's PT mapped,
 *  - pmm.bss_end: current thread's TCB mapped,
 *  - relas: array of OSZ_rela items alloceted with kalloc()
 *  - stack_ptr: physical address of thread's stack */
bool_t service_rtlink()
{
    int i, j, k, n = 0;
    OSZ_tcb *tcb = (OSZ_tcb*)(pmm.bss_end);
    OSZ_rela *rel = relas;
    uint64_t *paging = (uint64_t *)&tmpmap;

    /*** collect addresses to relocate ***/
    for(j=0; j<__PAGESIZE/8; j++) {
        Elf64_Ehdr *ehdr=(Elf64_Ehdr *)(paging[j]&~(__PAGESIZE-1)&~((uint64_t)1<<63));    
        if(ehdr==NULL || kmemcmp(ehdr->e_ident,ELFMAG,SELFMAG))
            continue;
        Elf64_Phdr *phdr=(Elf64_Phdr *)((uint8_t *)ehdr+ehdr->e_phoff);
        Elf64_Dyn *d;
        uint64_t got = 0;
        Elf64_Sym *sym, *s;
        Elf64_Rela *rela;
        // the string table
        char *strtable;
        int syment, relasz, relaent=24;
    
        if(j==0) {
            // record crt0 entry point at the begining of segment
            tcb->rip = (uint64_t)(TEXT_ADDRESS + ehdr->e_ehsize+ehdr->e_phnum*ehdr->e_phentsize);
        }

        /* Program header */
        for(i = 0; i < ehdr->e_phnum; i++){
            if(phdr->p_type==PT_DYNAMIC) {
                d = (Elf64_Dyn *)((uint8_t *)ehdr + phdr->p_offset);
                /* dynamic table */
#if DEBUG
                if(debug==DBG_RTIMPORT)
                    kprintf("  Import %x (%d bytes):\n",
                        phdr->p_offset + j*__PAGESIZE, (int)phdr->p_memsz
                    );
#endif
                while(d->d_tag != DT_NULL) {
                    switch(d->d_tag) {
                        case DT_STRTAB:
                            strtable = (char *)ehdr + d->d_un.d_ptr;
                            break;
                        case DT_SYMTAB:
                            sym = (Elf64_Sym *)((uint8_t *)ehdr + d->d_un.d_ptr);
                            break;
                        case DT_SYMENT:
                            syment = d->d_un.d_val;
                            break;
                        case DT_JMPREL:
                            rela = (Elf64_Rela *)((uint8_t *)ehdr + d->d_un.d_ptr);
                            break;
                        case DT_PLTRELSZ:
                            relasz = d->d_un.d_val;
                            break;
                        case DT_RELAENT:
                            relaent = d->d_un.d_val;
                            break;
                        case DT_PLTGOT:
                            got = d->d_un.d_ptr;
                            break;
                        default:
                            break;
                    }
                    /* move pointer to next dynamic entry */
                    d++;
                }
                break;
            }
            /* move pointer to next program header */
            phdr = (Elf64_Phdr *)((uint8_t *)phdr + ehdr->e_phentsize);
        }
    
        if(got) {
            /* GOT plt entries */
            for(i = 0; i < relasz / relaent; i++){
                // failsafe
                if(n >= 2*__PAGESIZE/sizeof(OSZ_rela))
                    break;
                s = (Elf64_Sym *)((char *)sym + ELF64_R_SYM(rela->r_info) * syment);
                /* get the physical address and sym from stringtable */
                uint64_t o =rela->r_offset + j*__PAGESIZE;
                /* because the thread is not mapped yet, we have to translate
                 * address manually */
                rel->offs = (paging[o/__PAGESIZE]&~(__PAGESIZE-1)&~((uint64_t)1<<63)) + (o&(__PAGESIZE-1));
                rel->sym = strtable + s->st_name;
#if DEBUG
                if(debug==DBG_RTIMPORT)
                    kprintf("    %x %s +%x?\n", rel->offs,
                        strtable + s->st_name, rela->r_addend
                    );
#endif
                n++; rel++;
                /* move pointer to next rela entry */
                rela = (Elf64_Rela *)((uint8_t *)rela + relaent);
            }
        }
    }

#if DEBUG
    if(debug==DBG_IRQ && irq_routing_table!=NULL)
        kprintf("IRQ Routing Table (%d IRQs, %d handlers per IRQ):\n", ISR_NUMIRQ, nrirqmax);
#endif

    /*** resolve addresses ***/
    for(j=0; j<__PAGESIZE/8; j++) {
        Elf64_Ehdr *ehdr=(Elf64_Ehdr *)(paging[j]&~(__PAGESIZE-1)&~((uint64_t)1<<63));    
        if(ehdr==NULL || kmemcmp(ehdr->e_ident,ELFMAG,SELFMAG))
            continue;
        Elf64_Phdr *phdr=(Elf64_Phdr *)((uint8_t *)ehdr+ehdr->e_phoff);
        Elf64_Dyn *d;
        Elf64_Sym *sym, *s;
        // the string table
        char *strtable;
        int strsz, syment;
        // clear session header offset, we don't need it, so we can reuse it
        // to store irq handler's address
        ehdr->e_shoff = 0;
    
        for(i = 0; i < ehdr->e_phnum; i++){
            if(phdr->p_type==PT_DYNAMIC) {
                d = (Elf64_Dyn *)((uint8_t *)ehdr + phdr->p_offset);
                while(d->d_tag != DT_NULL) {
                    switch(d->d_tag) {
                        case DT_STRTAB:
                            strtable = (char *)ehdr + d->d_un.d_ptr;
                            break;
                        case DT_STRSZ:
                            strsz = d->d_un.d_val;
                            break;
                        case DT_SYMTAB:
                            sym = (Elf64_Sym *)((uint8_t *)ehdr + d->d_un.d_ptr);
                            break;
                        case DT_SYMENT:
                            syment = d->d_un.d_val;
                            break;
                        default:
                            break;
                    }
                    /* move pointer to next dynamic entry */
                    d++;
                }
                break;
            }
            /* move pointer to next program header */
            phdr = (Elf64_Phdr *)((uint8_t *)phdr + ehdr->e_phentsize);
        }
        /* dynamic symbol table */
#if DEBUG
        if(debug==DBG_RTEXPORT)
            kprintf("  Export %x (%d bytes):\n",
                sym, strtable-(char*)sym
            );
#endif
        //foreach(sym)
        s=sym;
        for(i=0;i<(strtable-(char*)sym)/syment;i++) {
            if(s->st_name > strsz) break;
            // is it defined in this object?
            if(s->st_value) {
                uint64_t offs = (uint64_t)s->st_value + j*__PAGESIZE+TEXT_ADDRESS;
#if DEBUG
                if(debug==DBG_RTEXPORT)
                    kprintf("    %x %s:", offs, strtable + s->st_name);
#endif
                // parse irqX() symbols to fill up irq_routing_table
                if(irq_routing_table != NULL && !kmemcmp(strtable + s->st_name,"irq",3)) {
                    //at least one number
                    if(*(strtable + s->st_name + 3)>='0' && *(strtable + s->st_name + 3)<='9' &&
                        //strlen() = 4 | 5 | 6
                        (*(strtable + s->st_name + 4)==0 ||
                        *(strtable + s->st_name + 5)==0 ||
                        *(strtable + s->st_name + 6)==0)
                    ) {
                        env_dec((unsigned char*)strtable + s->st_name + 3, (uint*)&k, 0, 255-32);
                        service_installirq(k, offs);
                    } else {
                        // record the irq handler's offset for later, we cannot
                        // install it. We'll have to autodetect the irq
                        ehdr->e_shoff = offs;
                    }
                }
                // look up in relas array which addresses require
                // this symbol's virtual address
                for(k=0;k<n;k++){
                    OSZ_rela *r = (OSZ_rela*)((char *)relas + k*sizeof(OSZ_rela));
                    if(r->offs != 0 && !kstrcmp(r->sym, strtable + s->st_name)) {
#if DEBUG
                        if(debug==DBG_RTEXPORT)
                            kprintf(" %x", r->offs);
#endif
                        // save virtual address
                        uint64_t *ptr = (uint64_t *)r->offs;
                        *ptr = offs;
                        r->offs = 0;
                    }
                }
#if DEBUG
                if(debug==DBG_RTEXPORT)
                    kprintf("\n");
#endif
            }
            /* move pointer to next symbol */
            s = (Elf64_Sym *)((uint8_t *)s + syment);
        }
    }
    // check if we have resolved all references
    for(i=0;i<n;i++){
        OSZ_rela *r = (OSZ_rela*)((char *)relas + i*sizeof(OSZ_rela));
        if(r->offs!=0) {
            kpanic("shared library missing for %s()\n", r->sym);
        }
    }

    // push entry point as the last callback
    // crt0 starts with a 'ret', so _start function address
    // will be popped up and executed.

    // if we're linking system task, save it's entry point (no ELF header)
    if(irq_routing_table!=NULL) {
        // at TEXT_ADDRESS (_usercode) is a 'ret',
        // so main() comes after that
        *stack_ptr = TEXT_ADDRESS + (&_main - &_usercode);
        stack_ptr--;
        tcb->rsp -= 8;
    } else {
        Elf64_Ehdr *ehdr=(Elf64_Ehdr *)(paging[0]&~(__PAGESIZE-1)&~((uint64_t)1<<63));    
        if(ehdr==NULL || kmemcmp(ehdr->e_ident,ELFMAG,SELFMAG) || ehdr->e_phoff==ehdr->e_entry)
            kpanic("no executor?");
        *stack_ptr = ehdr->e_entry + TEXT_ADDRESS;
        stack_ptr--;
        tcb->rsp -= 8;
    }

    // go again (other way around) and save entry points onto stack,
    // but skip the first ELF as executor's entry point is already
    // on the stack
    for(j=__PAGESIZE/8-1; j>1; j--) {
        Elf64_Ehdr *ehdr=(Elf64_Ehdr *)(paging[j]&~(__PAGESIZE-1)&~((uint64_t)1<<63));    
        if(ehdr==NULL || kmemcmp(ehdr->e_ident,ELFMAG,SELFMAG))
            continue;
        *stack_ptr = ehdr->e_entry + TEXT_ADDRESS + j*__PAGESIZE;
        stack_ptr--;
        tcb->rsp -= 8;
    }
    // sanity check thread
    return thread_check(tcb, (phy_t*)paging);
}

void service_mapbss(uint64_t phys, uint64_t size)
{
}

/* Initialize a system service, save "fs" and "system" */
void service_init(int subsystem, char *fn)
{
    char *cmd = fn;
    while(cmd[0]!='/')
        cmd++;
    cmd++;
    pid_t pid = thread_new(cmd);
    services[-subsystem] = pid;
    // map executable
    if(service_loadelf(fn) == (void*)(-1)) {
#if DEBUG
        kprintf("WARNING unable to load ELF from %s\n", fn);
#endif
        return;
    }
    // map libc
    service_loadso("lib/libc.so");
    // dynamic linker
    if(service_rtlink()) {
        // add to queue so that scheduler will know about this thread
        sched_add((OSZ_tcb*)(pmm.bss_end));
    } else {
#if DEBUG
        kprintf("WARNING thread check failed for %s\n", fn);
#endif
    }
}

/* Initialize the file system service, the "fs" task */
void fs_init()
{
    char *s, *f, *drvs = (char *)fs_locate("etc/sys/drivers");
    char *drvs_end = drvs + fs_size;
    char fn[256];
    pid_t pid = thread_new("fs");
    services[-SRV_fs] = pid;
    // map file system dispatcher
    if(service_loadelf("sbin/fs") == (void*)(-1)) {
#if DEBUG
        kpanic("unable to load ELF from /sbin/fs");
#endif
    }
    // map libc
    service_loadso("lib/libc.so");
    // load file system drivers
    if(drvs==NULL) {
        // hardcoded if driver list not found
        // should not happen!
        service_loadso("lib/sys/fs/gpt.so");    // disk
        service_loadso("lib/sys/fs/fsz.so");    // initrd and OS/Z partitions
        service_loadso("lib/sys/fs/vfat.so");   // EFI boot partition
    } else {
        for(s=drvs;s<drvs_end;) {
            f = s; while(s<drvs_end && *s!=0 && *s!='\n') s++;
            if(f[0]=='*' && f[1]==9 && f[2]=='f' && f[3]=='s') {
                f+=2;
                if(s-f<255-8) {
                    kmemcpy(&fn[0], "lib/sys/", 8);
                    kmemcpy(&fn[8], f, s-f);
                    fn[s-f+8]=0;
                    service_loadso(fn);
                }
                continue;
            }
            // failsafe
            if(s>=drvs_end || *s==0) break;
            if(*s=='\n') s++;
        }
    }

    // dynamic linker
    if(service_rtlink()) {
        // map initrd in "fs" task's memory
        service_mapbss(bootboot.initrd_ptr, bootboot.initrd_size);

        // add to queue so that scheduler will know about this thread
        sched_add((OSZ_tcb*)(pmm.bss_end));
    } else {
        kpanic("thread check failed for /sbin/fs");
    }
}

/* Initialize the user interface service, the "ui" task */
void ui_init()
{
    service_init(SRV_ui, "sbin/ui");
}
