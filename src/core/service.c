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
 * @brief System service loader and ELF parser
 */

#include <elf.h>
#include <sys/sysinfo.h>
#include "env.h"

/* external resources */
extern uint8_t _code;
extern uint64_t *stack_ptr;
extern char *drvnames;
extern uint64_t *drivers;
extern uint64_t *drvptr;
extern phy_t screen[2];
extern phy_t pdpe;
extern uint64_t isr_entropy[4];
extern uint64_t *syslog_buf;
extern uint64_t freq;
extern sysinfo_t *sysinfostruc;
extern uint8_t sys_fault;

extern unsigned char *env_dec(unsigned char *s, uint *v, uint min, uint max);

typedef unsigned char *valist;
#define vastart(list, param) (list = (((valist)&param) + sizeof(void*)*8))
#define vaarg(list, type)    (*(type *)((list += sizeof(void*)) - sizeof(void*)))

/* we define the addresses as 32 bit, as code segment cannot grow
   above 4G. This way we can have 512 relocation records in a page */

/* memory allocated for relocation addresses */
OSZ_rela __attribute__ ((section (".data"))) *relas;

/* pids of services. Negative pids are service ids and looked up in this */
pid_t  __attribute__ ((section (".data"))) *services;
uint64_t __attribute__ ((section (".data"))) nrservices = -SRV_USRFIRST;
/* irq routing */
uint64_t __attribute__ ((section (".data"))) *irq_routing_table;
/* dynsym */
unsigned char __attribute__ ((section (".data"))) *nosym = (unsigned char*)"(no symbol)";
#if DEBUG
virt_t __attribute__ ((section (".data"))) lastsym;
#endif

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

#if DEBUG
    if(debug&DBG_ELF)
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
            syslog_early("WARNING corrupt ELF binary: %s", fn);
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
    /* GNU ld bug workaround: keep symtab and strtab */
    size++;
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
    if(debug&DBG_ELF)
        kprintf("  ");
#endif
    service_loadelf(fn);
}

#if DEBUG
virt_t service_lookupsym(uchar *name, size_t size)
{
    OSZ_tcb *tcb = (OSZ_tcb*)0;
    Elf64_Ehdr *ehdr;
    /* common to all threads */
    virt_t addr = CORE_ADDRESS;
    if(size==3 && !kmemcmp(name,"tcb",3))
        return 0;
    if(size==2 && !kmemcmp(name,"mq",2))
        return __PAGESIZE;
    if(size==5 && !kmemcmp(name,"stack",5))
        return __SLOTSIZE/2;
    if(size==4 && !kmemcmp(name,"text",4))
        return TEXT_ADDRESS;
    if(size==3 && !kmemcmp(name,"bss",3))
        return BSS_ADDRESS;
    if(size==4 && !kmemcmp(name,"core",4))
        return CORE_ADDRESS;
    /* SYS task only */
    if(tcb->memroot == sys_mapping) {
        if(size==5 && !kmemcmp(name,"_main",5))
            return TEXT_ADDRESS;
        if(size==3 && !kmemcmp(name,"irt",3))
            return TEXT_ADDRESS + __PAGESIZE;
    }
    /* otherwise look for elf and parse for symbols */
        sys_fault = false;
    while(!sys_fault && (addr == CORE_ADDRESS || addr < BSS_ADDRESS)) {
        if(!kmemcmp(((Elf64_Ehdr*)addr)->e_ident,ELFMAG,SELFMAG)){
            ehdr = (Elf64_Ehdr*)addr;
            Elf64_Phdr *phdr=(Elf64_Phdr *)((uint8_t *)ehdr+(uint32_t)ehdr->e_phoff);
            Elf64_Sym *sym=NULL, *s;
            // the string tables
            char *strtable;
            int i, strsz, syment;

            /* Program Header */
            for(i = 0; i < ehdr->e_phnum; i++){
                /* we only need the Dynamic header */
                if(phdr->p_type==PT_DYNAMIC) {
                    /* Dynamic header */
                    Elf64_Dyn *d;
                    d = (Elf64_Dyn *)((uint32_t)phdr->p_offset+(uint64_t)ehdr);
                    while(d->d_tag != DT_NULL) {
                        if(d->d_tag == DT_STRTAB) {
                            strtable = (char*)ehdr + (((uint32_t)d->d_un.d_ptr)&0xFFFFFF);
                        }
                        if(d->d_tag == DT_STRSZ) {
                            strsz = d->d_un.d_val;
                        }
                        if(d->d_tag == DT_SYMTAB) {
                            sym = (Elf64_Sym *)((uint8_t*)ehdr + (((uint32_t)d->d_un.d_ptr)&0xFFFFFF));
                        }
                        if(d->d_tag == DT_SYMENT) {
                            syment = d->d_un.d_val;
                        }
                        /* move pointer to next dynamic entry */
                        d++;
                    }
                }
                /* move pointer to next section header */
                phdr = (Elf64_Phdr *)((uint8_t *)phdr + ehdr->e_phentsize);
            }
            /* fallback to sections if dynamic segment not found */
            if(sym==NULL) {
                // section header
                Elf64_Shdr *shdr=(Elf64_Shdr *)((uint8_t *)ehdr+ehdr->e_shoff);
                // string table and other section header entries
                Elf64_Shdr *strt=(Elf64_Shdr *)((uint8_t *)shdr+(uint64_t)ehdr->e_shstrndx*(uint64_t)ehdr->e_shentsize);
                char *shstr = (char*)ehdr + (uint32_t)strt->sh_offset;
                // failsafe
                if((virt_t)strt<TEXT_ADDRESS || ((virt_t)strt>=BSS_ADDRESS && (virt_t)strt<FBUF_ADDRESS) ||
                   (virt_t)shstr<TEXT_ADDRESS || ((virt_t)shstr>=BSS_ADDRESS && (virt_t)shstr<FBUF_ADDRESS)
                )
                    continue;
                /* Section header */
                for(i = 0; i < ehdr->e_shnum; i++){
                    if(!kstrcmp(shstr+(uint32_t)shdr->sh_name, ".symtab")){
                        sym = (Elf64_Sym *)((uint8_t*)ehdr + (uint32_t)shdr->sh_offset);
                        syment = (int)shdr->sh_entsize;
                    }
                    if(!kstrcmp(shstr+(uint32_t)shdr->sh_name, ".strtab")){
                        strtable = (char*)ehdr + (uint32_t)shdr->sh_offset;
                        strsz = (int)shdr->sh_size;
                    }
                    /* move pointer to next section header */
                    shdr = (Elf64_Shdr *)((uint8_t *)shdr + ehdr->e_shentsize);
                }
            }
            if(sym==NULL)
                continue;

            /* find the symbol with that name */
            s=sym;
            for(i=0;i<(strtable-(char*)sym)/syment;i++) {
                if(s->st_name > strsz) break;
                if( (char*)(strtable + (uint32_t)s->st_name)!=0 &&
                    *((char*)(strtable + (uint32_t)s->st_name + size))==0 &&
                    !kmemcmp(strtable + (uint32_t)s->st_name,name,size)
                ) {
                    return (virt_t)(s->st_value + (addr!=CORE_ADDRESS?(uint64_t)ehdr:0));
                }
                s++;
            }
        }
        if(addr == CORE_ADDRESS)
            addr = TEXT_ADDRESS;
        else
            addr += __PAGESIZE;
    }
    return 0;
}
#endif

/* look up a symbol for address */
uchar *service_sym(virt_t addr)
{
    OSZ_tcb *tcb = (OSZ_tcb*)0;
    Elf64_Ehdr *ehdr;
    uint64_t ptr;

#if DEBUG
    lastsym = TEXT_ADDRESS;
#endif
    /* rule out non text address ranges */
    if(addr < TEXT_ADDRESS || (addr >= BSS_ADDRESS && addr < (virt_t)CORE_ADDRESS))
        return nosym;
    /* find the elf header for the address */
    ptr = addr;
    ptr &= ~(__PAGESIZE-1);
    sys_fault = false;
    while(!sys_fault && ptr>TEXT_ADDRESS && kmemcmp(((Elf64_Ehdr*)ptr)->e_ident,ELFMAG,SELFMAG))
        ptr -= __PAGESIZE;
    /* one special case, SYS task, which does not have an elf header */
    if(ptr == TEXT_ADDRESS && tcb->memroot == sys_mapping) {
        if(addr<TEXT_ADDRESS+__PAGESIZE)
            return (uchar*)"_main (irq dispatcher)";
        if(addr>=TEXT_ADDRESS+__PAGESIZE && addr<=TEXT_ADDRESS+3*__PAGESIZE) {
#if DEBUG
            lastsym = TEXT_ADDRESS + __PAGESIZE;
#endif
            return (uchar*)"irt";
        }
        return nosym;
    }
    ehdr = (Elf64_Ehdr*)ptr;
    /* failsafe */
    if(sys_fault || (uint64_t)ehdr < (uint64_t)TEXT_ADDRESS || kmemcmp(ehdr->e_ident,ELFMAG,SELFMAG))
        return nosym;
    if(addr<FBUF_ADDRESS)
        addr -= (virt_t)ehdr;
    Elf64_Phdr *phdr=(Elf64_Phdr *)((uint8_t *)ehdr+(uint32_t)ehdr->e_phoff);
    Elf64_Sym *sym=NULL, *s;
    // the string tables
    char *strtable, *last=NULL;
    int i, strsz, syment;

    /* Program Header */
    for(i = 0; i < ehdr->e_phnum; i++){
        /* we only need the Dynamic header */
        if(phdr->p_type==PT_DYNAMIC) {
            /* Dynamic header */
            Elf64_Dyn *d;
            d = (Elf64_Dyn *)((uint32_t)phdr->p_offset+(uint64_t)ehdr);
            while(d->d_tag != DT_NULL) {
                if(d->d_tag == DT_STRTAB) {
                    strtable = (char*)ehdr + (((uint32_t)d->d_un.d_ptr)&0xFFFFFF);
                }
                if(d->d_tag == DT_STRSZ) {
                    strsz = d->d_un.d_val;
                }
                if(d->d_tag == DT_SYMTAB) {
                    sym = (Elf64_Sym *)((uint8_t*)ehdr + (((uint32_t)d->d_un.d_ptr)&0xFFFFFF));
                }
                if(d->d_tag == DT_SYMENT) {
                    syment = d->d_un.d_val;
                }
                /* move pointer to next dynamic entry */
                d++;
            }
        }
        /* move pointer to next section header */
        phdr = (Elf64_Phdr *)((uint8_t *)phdr + ehdr->e_phentsize);
    }
    /* fallback to sections if dynamic segment not found */
    if(sym==NULL) {
        // section header
        Elf64_Shdr *shdr=(Elf64_Shdr *)((uint8_t *)ehdr+ehdr->e_shoff);
        // string table and other section header entries
        Elf64_Shdr *strt=(Elf64_Shdr *)((uint8_t *)shdr+(uint64_t)ehdr->e_shstrndx*(uint64_t)ehdr->e_shentsize);
        char *shstr = (char*)ehdr + (uint32_t)strt->sh_offset;
        // failsafe
        if((virt_t)strt<TEXT_ADDRESS || ((virt_t)strt>=BSS_ADDRESS && (virt_t)strt<FBUF_ADDRESS) ||
           (virt_t)shstr<TEXT_ADDRESS || ((virt_t)shstr>=BSS_ADDRESS && (virt_t)shstr<FBUF_ADDRESS)
        )
            return nosym;
        /* Section header */
        for(i = 0; i < ehdr->e_shnum; i++){
            if(!kstrcmp(shstr+(uint32_t)shdr->sh_name, ".symtab")){
                sym = (Elf64_Sym *)((uint8_t*)ehdr + (uint32_t)shdr->sh_offset);
                syment = (int)shdr->sh_entsize;
            }
            if(!kstrcmp(shstr+(uint32_t)shdr->sh_name, ".strtab")){
                strtable = (char*)ehdr + (uint32_t)shdr->sh_offset;
                strsz = (int)shdr->sh_size;
            }
            /* move pointer to next section header */
            shdr = (Elf64_Shdr *)((uint8_t *)shdr + ehdr->e_shentsize);
        }
    }
    if(sym==NULL)
        return nosym;

    /* find the closest symbol to the address */
    s=sym;
    ptr = 0;
    for(i=0;i<(strtable-(char*)sym)/syment;i++) {
        if(s->st_name > strsz) break;
        if((virt_t)s->st_value <= (virt_t)addr &&
           (virt_t)s->st_value > ptr &&
           (char*)(strtable + (uint32_t)s->st_name)!=0)
        {
            last = strtable + (uint32_t)s->st_name;
#if DEBUG
            lastsym = (uint64_t)s->st_value;
#endif
            ptr = s->st_value;
        }
        s++;
    }
    return last!=NULL && last[0]!=0 ? (uchar*)last : (uchar*)nosym;
}

c_assert(ISR_NUMIRQ < 224);

void service_installirq(uint8_t irq, uint64_t offs)
{
    uint k=((irq)*nrirqmax)+1;
    uint last=(irq+1)*nrirqmax;
    if(irq>=0 && irq<ISR_NUMIRQ && k<ISR_NUMIRQ*nrirqmax-1) {
        // find next free slot
        while(irq_routing_table[k]!=0&&k<last) k++;
#if DEBUG
        if(debug&DBG_IRQ)
            kprintf("  IRQ %3d: irt[%4d]=%4x %s\n",
                irq, k, offs, drivers[(offs-TEXT_ADDRESS)/__PAGESIZE]);
#endif
        syslog_early(" %3d: irt[%4d]=%4x %s",
            irq, k, offs, drivers[(offs-TEXT_ADDRESS)/__PAGESIZE]);
        if(irq_routing_table[k]!=0)
            syslog_early("too many (%d+) IRQ handlers for %d\n", nrirqmax, irq);
        irq_routing_table[k] = offs;
    } else {
            syslog_early("WARNING irq_routing_table[%d] for %x out of range", k, offs);
    }
}

/* Fill in GOT entries. Relies on identity mapping
 *  - tmpmap: the text segment's PT mapped,
 *  - pmm.bss_end: current thread's TCB mapped,
 *  - relas: array of OSZ_rela items alloceted with kalloc()
 *  - stack_ptr: physical address of thread's stack */
bool_t service_rtlink()
{
    int i, j, k, l = -1, n = 0;
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
        Elf64_Rela *relad;
        // the string table
        char *strtable;
        int syment, relasz=0, reladsz=0, relaent=24;

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
                if(debug&DBG_RTIMPORT)
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
                        case DT_RELA:
                            relad = (Elf64_Rela *)((uint8_t *)ehdr + d->d_un.d_ptr);
                            break;
                        case DT_RELASZ:
                            reladsz = d->d_un.d_val;
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
           /* GOT data entries */
            for(i = 0; i < reladsz / relaent; i++){
                // failsafe
                if(n >= 2*__PAGESIZE/sizeof(OSZ_rela))
                    break;
                s = (Elf64_Sym *)((char *)sym + ELF64_R_SYM(relad->r_info) * syment);
                /* get the physical address and sym from stringtable */
                uint64_t o = (uint64_t)((int64_t)relad->r_offset + j*__PAGESIZE + (int64_t)relad->r_addend);
                /* because the thread is not mapped yet, we have to translate
                 * address manually */
                rel->offs = (paging[o/__PAGESIZE]&~(__PAGESIZE-1)&~((uint64_t)1<<63)) + (o&(__PAGESIZE-1));
                rel->sym = strtable + s->st_name;
#if DEBUG
                if(debug&DBG_RTIMPORT)
                    kprintf("    %x D %s +%x?\n", rel->offs,
                        strtable + s->st_name, relad->r_addend
                    );
#endif
                n++; rel++;
                /* move pointer to next rela entry */
                relad = (Elf64_Rela *)((uint8_t *)relad + relaent);
            }
            /* GOT plt entries */
            for(i = 0; i < relasz / relaent; i++){
                // failsafe
                if(n >= 2*__PAGESIZE/sizeof(OSZ_rela))
                    break;
                s = (Elf64_Sym *)((char *)sym + ELF64_R_SYM(rela->r_info) * syment);
                /* get the physical address and sym from stringtable */
                uint64_t o = (uint64_t)((int64_t)rela->r_offset + j*__PAGESIZE + (int64_t)rela->r_addend);
                /* because the thread is not mapped yet, we have to translate
                 * address manually */
                rel->offs = (paging[o/__PAGESIZE]&~(__PAGESIZE-1)&~((uint64_t)1<<63)) + (o&(__PAGESIZE-1));
                rel->sym = strtable + s->st_name;
#if DEBUG
                if(debug&DBG_RTIMPORT)
                    kprintf("    %x T %s +%x?\n", rel->offs,
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
    if((debug&DBG_IRQ) && irq_routing_table!=NULL)
        kprintf("\nIRQ Routing Table (%d IRQs, %d handlers per IRQ):\n", ISR_NUMIRQ, nrirqmax);
#endif
    if(irq_routing_table!=NULL)
        syslog_early("IRQ Routing Table (%d IRQs, %d handlers per IRQ)", ISR_NUMIRQ, nrirqmax);

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
        if(debug&DBG_RTEXPORT)
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
                if(debug&DBG_RTEXPORT)
                    kprintf("    %x %s:", offs, strtable + s->st_name);
#endif
                // parse irqX() symbols to fill up irq_routing_table
                if(irq_routing_table != NULL) {
                    if(!kmemcmp(strtable + s->st_name,"irq",3)) {
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
                            // install it yet. We'll have to autodetect the irq number
                            ehdr->e_shoff = offs;
                        }
                    }
                    if(!kmemcmp(strtable + s->st_name,"mem2vid",8)) {
#if DEBUG
                        if(debug&DBG_IRQ)
                            kprintf("  IRQ m2v: irt[%4d]=%4x %s\n",
                                (ISR_NUMIRQ*nrirqmax)+1, offs, drivers[(offs-TEXT_ADDRESS)/__PAGESIZE]);
#endif
                        syslog_early(" m2v: irt[%4d]=%4x %s",
                                (ISR_NUMIRQ*nrirqmax)+1, offs, drivers[(offs-TEXT_ADDRESS)/__PAGESIZE]);
                        irq_routing_table[(ISR_NUMIRQ*nrirqmax)+1] = offs;
                    }
                    if(!kmemcmp(strtable + s->st_name,"tmrfreq",8)) {
                        freq = *((uint64_t*)((char*)ehdr + s->st_value));
                        l = j;
                    }
                }
                // look up in relas array which addresses require
                // this symbol's virtual address
                for(k=0;k<n;k++){
                    OSZ_rela *r = (OSZ_rela*)((char *)relas + k*sizeof(OSZ_rela));
                    if(r->offs != 0 && !kstrcmp(r->sym, strtable + s->st_name)) {
#if DEBUG
                        if(debug&DBG_RTEXPORT)
                            kprintf(" %x", r->offs);
#endif
                        // save virtual address
                        uint64_t *ptr = (uint64_t *)r->offs;
                        *ptr = offs;
                        r->offs = 0;
                    }
                }
#if DEBUG
                if(debug&DBG_RTEXPORT)
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
    if(l!=-1)
        syslog_early("Timer: frequency set by %s\n", drivers[l]);

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
    // pushed on the stack
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

/* Initialize a non-critical system service */
void service_init(int subsystem, char *fn)
{
    char *cmd = fn;
    while(cmd[0]!='/')
        cmd++;
    cmd++;
    pid_t pid = thread_new(cmd);
    // map executable
    if(service_loadelf(fn) == (void*)(-1)) {
        syslog_early("WARNING unable to load ELF from %s", fn);
        return;
    }
    // map libc
    service_loadso("lib/libc.so");
    // dynamic linker
    if(service_rtlink()) {
        //set priority for the task
        if(subsystem == SRV_USRFIRST)
            ((OSZ_tcb*)(pmm.bss_end))->priority = PRI_APP;    // rescue shell
        else if(subsystem == SRV_USRLAST)
            ((OSZ_tcb*)(pmm.bss_end))->priority = PRI_IDLE;   // screen saver
        else
            ((OSZ_tcb*)(pmm.bss_end))->priority = PRI_SRV;    // everything else
        // add to queue so that scheduler will know about this thread
        sched_add((OSZ_tcb*)(pmm.bss_end));

        if(subsystem > SRV_USRFIRST) {
            services[-subsystem] = pid;
            syslog_early("Service -%d \"%s\" registered as %x",-subsystem,cmd,pid);
        }
    } else {
        syslog_early("WARNING thread check failed for %s", fn);
    }
}

/* Initialize the file system service, the "fs" task */
void fs_init()
{
    char *s, *f, *drvs = (char *)fs_locate("etc/sys/drivers");
    char *drvs_end = drvs + fs_size;
    char fn[256];
    pid_t pid = thread_new("FS");
    // map file system dispatcher
    if(service_loadelf("sbin/fs") == (void*)(-1)) {
        kpanic("unable to load ELF from /sbin/fs");
    }
    // map libc
    service_loadso("lib/libc.so");
    // load file system drivers
    if(drvs==NULL) {
        // hardcoded if driver list not found
        // should not happen!
        syslog_early("/etc/sys/drivers not found");
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
        vmm_mapbss((OSZ_tcb*)(pmm.bss_end),BSS_ADDRESS + __SLOTSIZE,bootboot.initrd_ptr, bootboot.initrd_size, PG_USER_RW);

        // add to queue so that scheduler will know about this thread
        ((OSZ_tcb*)(pmm.bss_end))->priority = PRI_SRV;
        sched_add((OSZ_tcb*)(pmm.bss_end));

        services[-SRV_FS] = pid;
        syslog_early("Service -%d \"%s\" registered as %x",-SRV_FS,"FS",pid);
    } else {
        kpanic("thread check failed for /sbin/fs");
    }
}

/* Initialize the user interface service, the "ui" task */
void ui_init()
{
    pid_t pid = thread_new("UI");

    // map user interface code
    if(service_loadelf("sbin/ui") == (void*)(-1)) {
        kpanic("unable to load ELF from /sbin/ui");
    }
    // map libc
    service_loadso("lib/libc.so");
    // map window decorator
//    service_loadso("lib/ui/decor.so");
    // dynamic linker
    if(service_rtlink()) {
        // allocate and map screen buffer B
        int i;
        phy_t bss = sysinfostruc->screen_ptr;
        i = ((bootboot.fb_width * bootboot.fb_height * 4 +
            __SLOTSIZE - 1) / __SLOTSIZE) * (display>=DSP_STEREO_MONO?2:1);
        while(i-->0) {
            vmm_mapbss((OSZ_tcb*)(pmm.bss_end),bss, (phy_t)pmm_allocslot(), __SLOTSIZE, PG_USER_RW);
            if(!screen[1]) {
                screen[1]=pdpe;
            }
            bss += __SLOTSIZE;
        }

        // add to queue so that scheduler will know about this thread
        ((OSZ_tcb*)(pmm.bss_end))->priority = PRI_SRV;
        sched_add((OSZ_tcb*)(pmm.bss_end));

        services[-SRV_UI] = pid;
        syslog_early("Service -%d \"%s\" registered as %x",-SRV_UI,"UI",pid);
    } else {
        kpanic("thread check failed for /sbin/ui");
    }
}

/* Initialize the system logger service, the "syslog" task */
void syslog_init()
{
    int i=0,j=0;
    uint64_t *paging = (uint64_t *)&tmpmap;
    pid_t pid = thread_new("syslog");
    // map file system dispatcher
    if(service_loadelf("sbin/syslog") == (void*)(-1)) {
        syslog_early("unable to load ELF from /sbin/syslog");
    }
    // map early syslog buffer
    for(i=0;paging[i]!=0;i++);
    for(j=0;j<nrlogmax;j++)
        paging[i+j] = ((*((uint64_t*)kmap_getpte(
            (uint64_t)syslog_buf + j*__PAGESIZE)))
            & ~(__PAGESIZE-1)) + PG_USER_RO;

    // map libc
    service_loadso("lib/libc.so");

    // dynamic linker
    if(service_rtlink()) {
        // add to queue so that scheduler will know about this thread
        ((OSZ_tcb*)(pmm.bss_end))->priority = PRI_SRV;
        sched_add((OSZ_tcb*)(pmm.bss_end));

        services[-SRV_syslog] = pid;
        syslog_early("Service -%d \"%s\" registered as %x",-SRV_syslog,"syslog",pid);
    } else {
        kprintf("WARNING thread check failed for /sbin/syslog\n");
    }
}
