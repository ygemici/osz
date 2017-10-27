/*
 * core/elf.c
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
 * @brief Early ELF64 loader and parser, used by service.c.
 */

#include <arch.h>
#include <elf.h>
#include "env.h"

/* external resources */
extern char osver[];
extern uint8_t _code, sys_fault, idle_first, scrptr;
extern uint64_t fstab, fstab_size, alarmstep, bogomips, lastpt, coreerrno;
extern uint64_t buffer_ptr, mmio_ptr, env_ptr;

extern unsigned char *env_dec(unsigned char *s, uint *v, uint min, uint max);

/* we define the addresses as 32 bit, as code segment cannot grow
   above 4G. This way we can have 512 relocation records in a page */

/* memory allocated for relocation addresses */
rela_t *relas;
unsigned char *nosym = (unsigned char*)"(no symbol)";
unsigned char *mqsym = (unsigned char*)"(mq)";
unsigned char *lssym = (unsigned char*)"(local stack)";
#define SOMAXNAME 32
char *loadedelf = NULL;

#if DEBUG
extern virt_t lastsym;
#endif

/**
 * load an ELF64 binary into text segment starting at 2M
 */
void *elf_load(char *fn)
{
    Elf64_Ehdr *elf=NULL;
    tcb_t *tcb = (tcb_t*)&tmpmap;
    uint64_t i, l, size, ret = lastpt, *paging = (uint64_t *)VMM_ADDRESS;
    void *ptr;
    Elf64_Dyn *d;
    char lib[SOMAXNAME+9], *strtable;
    kstrcpy((char*)&lib,"sys/lib/");
    // failsafe
    if(fn==NULL || fn[0]==0)
        return NULL;
    if(kstrlen(fn)+1>=SOMAXNAME)
        kpanic("Filename too long: %s", fn);
    // this is so early, we don't have fs task, so we use fs_locate()
    /* locate elf on initrd */
    elf=(Elf64_Ehdr *)fs_locate(fn);
    if(elf==NULL || kmemcmp(elf->e_ident,ELFMAG,SELFMAG)) goto elferr;
    srand[(lastpt+0)%4] *= 16807;
    srand[(lastpt+1)%4] ^= (uint64_t)elf;
    srand[(lastpt+2)%4] *= 16807;
    srand[(lastpt+3)%4] ^= elf->e_shoff<<16;
    /* get program headers */
    Elf64_Phdr *phdr_c=(Elf64_Phdr *)((uint8_t *)elf+elf->e_phoff);
    Elf64_Phdr *phdr_d=(Elf64_Phdr *)((uint8_t *)elf+elf->e_phoff+1*elf->e_phentsize);
    Elf64_Phdr *phdr_l=(Elf64_Phdr *)((uint8_t *)elf+elf->e_phoff+2*elf->e_phentsize);
    // valid elf for this platform?
    if( elf->e_ident[EI_CLASS]!=ELFCLASS64 || elf->e_ident[EI_DATA]!=ELFDATA2LSB ||
        elf->e_machine!=ARCH_ELFEM || elf->e_phnum<3 ||
        // code segment, page aligned, readable, executable, but not writable
        phdr_c->p_type!=PT_LOAD || phdr_c->p_paddr!=0 || (phdr_c->p_flags&PF_W)!=0 ||
        // data segment, page aligned, readable, writeable, but not executable
        phdr_d->p_type!=PT_LOAD || (phdr_d->p_vaddr&(__PAGESIZE-1))!=0 || (phdr_d->p_flags&PF_X)!=0 ||
        // linkage information
        phdr_l->p_type!=PT_DYNAMIC
        ) {
elferr:
            if(idle_first)
                kpanic("Missing or corrupt ELF: %s", fn);
#if DEBUG
            kprintf("Missing or corrupt ELF: %s\n", fn);
#endif
            return (void*)(-1);
    }
#if DEBUG
    if(debug&DBG_ELF)
        kprintf("  loadelf %x (%d+%d pages) @%d %s\n",elf,
            (phdr_c->p_memsz+__PAGESIZE-1)/__PAGESIZE,(phdr_d->p_memsz+__PAGESIZE-1)/__PAGESIZE,
            lastpt,fn);
#endif
    /* clear autodetected IRQ number. Reuse otherwise unused field here */
    elf->e_version=0xFFFF;
    /* map text segment */
    vmm_mapbuf(elf, (phdr_c->p_memsz+__PAGESIZE-1)/__PAGESIZE, PG_USER_RO);
    /* allocate data segment, page aligned */
    size=(phdr_d->p_memsz+__PAGESIZE-1)/__PAGESIZE;
    l=phdr_d->p_filesz;
    /* GNU ld bug workaround: keep symtab and strtab */
    l+=__PAGESIZE; if(l>phdr_d->p_memsz) l=phdr_d->p_memsz;
    for(i=0;i<size;i++) {
        vmm_checklastpt();
        ptr=pmm_alloc(1);
        if(l>0) {
            vmm_map((virt_t)&tmp3map, (phy_t)ptr, PG_CORE_NOCACHE|PG_PAGE);
            kmemcpy(&tmp3map, (uint8_t*)elf + phdr_d->p_paddr + i*__PAGESIZE, l>__PAGESIZE?__PAGESIZE:l);
            l-=__PAGESIZE;
        }
        paging[lastpt++]=((uint64_t)ptr&~(__PAGESIZE-1))|PG_PAGE|PG_USER_RW|1UL<<PG_NX_BIT;
        tcb->allocmem++;
    }
    /* dynamic table, parse needed so records */
    d = (Elf64_Dyn *)((uint8_t *)elf + phdr_l->p_offset);
    while(d->d_tag != DT_NULL) {
        if(d->d_tag==DT_STRTAB) {
            strtable = (char *)elf + d->d_un.d_ptr;
            break;
        }
        d++;
    }
    d = (Elf64_Dyn *)((uint8_t *)elf + phdr_l->p_offset);
    while(d->d_tag != DT_NULL) {
        if(d->d_tag==DT_NEEDED) {
            // we assume here that all filenames are shorter than 32 bytes
            // which is true for the initrd
            for(i=0;i<__PAGESIZE-SOMAXNAME && loadedelf[i]!=0;i+=SOMAXNAME)
                if(!kstrcmp(strtable + d->d_un.d_ptr, loadedelf+i))
                    goto loaded;
            kstrcpy(loadedelf + i, strtable + d->d_un.d_ptr);
            kstrcpy((char*)&lib[8],strtable + d->d_un.d_ptr);
            elf_loadso(lib);
        }
loaded:
        /* move pointer to next dynamic entry */
        d++;
    }

    /* return start offset within text segment */
    return (void*)(ret * __PAGESIZE);
}

/**
 * load an ELF64 shared library
 */
inline void elf_loadso(char *fn)
{
#if DEBUG
    if(debug&DBG_ELF)
        kprintf("  ");
#endif
    elf_load(fn);
}

#if DEBUG
/**
 * look up symbol, string -> address
 */
virt_t elf_lookupsym(uchar *name, size_t size)
{
    Elf64_Ehdr *ehdr;
    /* common to all tasks */
    virt_t addr = TEXT_ADDRESS;
    if(size==3 && !kmemcmp(name,"tcb",4))    return 0;
    if(size==2 && !kmemcmp(name,"mq",3))     return __PAGESIZE;
    if(size==5 && !kmemcmp(name,"stack",6))  return __SLOTSIZE/2;
    if(size==4 && !kmemcmp(name,"text",5))   return TEXT_ADDRESS;
    if(size==3 && !kmemcmp(name,"bss",4))    return BSS_ADDRESS;
    if(size==4 && !kmemcmp(name,"sbss",4))   return SBSS_ADDRESS;
    if(size==6 && !kmemcmp(name,"shared",6)) return SBSS_ADDRESS;
    if(size==4 && !kmemcmp(name,"core",5))   return CORE_ADDRESS;
    if(size==4 && !kmemcmp(name,"buff",5))   return BUF_ADDRESS;
    /* otherwise look for elf and parse for symbols */
    sys_fault = false;
    while(!sys_fault && (addr == CORE_ADDRESS || addr < BSS_ADDRESS)) {
        if(!kmemcmp(((Elf64_Ehdr*)addr)->e_ident,ELFMAG,SELFMAG)){
            ehdr = (Elf64_Ehdr*)addr;
            Elf64_Phdr *phdr=(Elf64_Phdr *)((uint8_t *)ehdr+(uint32_t)ehdr->e_phoff);
            Elf64_Sym *sym=NULL, *s;
            // the string tables
            char *strtable=NULL;
            int i, strsz=0, syment=24;

            // section header
            Elf64_Shdr *shdr=(Elf64_Shdr *)((uint8_t *)ehdr+ehdr->e_shoff);
            // string table and other section header entries
            Elf64_Shdr *strt=(Elf64_Shdr *)((uint8_t *)shdr+(uint64_t)ehdr->e_shstrndx*(uint64_t)ehdr->e_shentsize);
            char *shstr = (char*)ehdr + (uint32_t)strt->sh_offset;
            // failsafe
            if((virt_t)strt<TEXT_ADDRESS || ((virt_t)strt>=BSS_ADDRESS && (virt_t)strt<FBUF_ADDRESS) ||
               (virt_t)shstr<TEXT_ADDRESS || ((virt_t)shstr>=BSS_ADDRESS && (virt_t)shstr<FBUF_ADDRESS)
            )
                sym=NULL;
            else {
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
            /* fallback to dynamic segment if symtab section not found */
            if(sym==NULL || strtable==NULL || strsz==0) {
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
            }
            if(sym==NULL || strtable==NULL || strsz==0)
                continue;

            /* find the symbol with that name */
            s=sym;
            for(i=0;i<(strtable-(char*)sym)/syment;i++) {
                if(s->st_name > strsz) break;
                if( s->st_value && (char*)(strtable + (uint32_t)s->st_name)!=0 &&
                    *((char*)(strtable + (uint32_t)s->st_name + size))==0 &&
                    !kmemcmp(strtable + (uint32_t)s->st_name,name,size)
                ) {
                    return (virt_t)(s->st_value + (addr!=CORE_ADDRESS?(uint64_t)ehdr:0));
                }
                s++;
            }
        }
        if(addr == CORE_ADDRESS)
            break;
        addr += __PAGESIZE;
        if(addr == BSS_ADDRESS)
            addr = CORE_ADDRESS;
    }
    return 0;
}
#endif

/**
 * look up a symbol for address, address -> string
 */
uchar *elf_sym(virt_t addr)
{
    Elf64_Ehdr *ehdr;
    uint64_t ptr;

#if DEBUG
    lastsym = TEXT_ADDRESS;
#endif
    /* rule out non text address ranges */
    if(addr >= __PAGESIZE && addr < __SLOTSIZE/2)
        return mqsym;
    if(addr >= __SLOTSIZE/2 && addr < __SLOTSIZE)
        return lssym;
    if(addr < TEXT_ADDRESS || (addr >= BSS_ADDRESS && addr < (virt_t)CORE_ADDRESS))
        return nosym;
    /* find the elf header for the address */
    ptr = addr;
    ptr &= ~(__PAGESIZE-1);
    sys_fault = false;
    while(!sys_fault && ptr>TEXT_ADDRESS && kmemcmp(((Elf64_Ehdr*)ptr)->e_ident,ELFMAG,SELFMAG))
        ptr -= __PAGESIZE;
    ehdr = (Elf64_Ehdr*)ptr;
    /* failsafe */
    if(sys_fault || ptr < TEXT_ADDRESS || kmemcmp(ehdr->e_ident,ELFMAG,SELFMAG)) {
        return nosym;
    }
    if(addr<FBUF_ADDRESS)
        addr -= (virt_t)ehdr;
    Elf64_Phdr *phdr=(Elf64_Phdr *)((uint8_t *)ehdr+(uint32_t)ehdr->e_phoff);
    Elf64_Sym *sym=NULL, *s;
    // the string tables
    char *strtable, *last=NULL;
    int i, strsz=0, syment=24;

    /* Section Headers */
    Elf64_Shdr *shdr=(Elf64_Shdr *)((uint8_t *)ehdr+(uint32_t)ehdr->e_shoff);
    // string table and other section header entries
    Elf64_Shdr *strt=(Elf64_Shdr *)((uint8_t *)shdr+(uint64_t)(ehdr->e_shstrndx)*(uint64_t)(ehdr->e_shentsize));
    sys_fault=false;
    char *shstr = (char*)ehdr + strt->sh_offset;
    // failsafe

    if(sys_fault || (virt_t)strt<TEXT_ADDRESS || ((virt_t)strt>=BSS_ADDRESS && (virt_t)strt<FBUF_ADDRESS) ||
       (virt_t)shstr<TEXT_ADDRESS || ((virt_t)shstr>=BSS_ADDRESS && (virt_t)shstr<FBUF_ADDRESS)
    )
        sym=NULL;
    else {
        /* Section header */
        for(i = 0; i < ehdr->e_shnum; i++){
            if(!kmemcmp(shstr+(uint32_t)shdr->sh_name, ".symtab",8)){
                sym = (Elf64_Sym *)((uint8_t*)ehdr + (uint32_t)shdr->sh_offset);
                syment = (int)shdr->sh_entsize;
            }
            if(!kmemcmp(shstr+(uint32_t)shdr->sh_name, ".strtab",8)){
                strtable = (char*)ehdr + (uint32_t)shdr->sh_offset;
                strsz = (int)shdr->sh_size;
            }
            /* move pointer to next section header */
            shdr = (Elf64_Shdr *)((uint8_t *)shdr + ehdr->e_shentsize);
        }
    }
    /* fallback to dynamic segment if symtab section not found */
    if(sym==NULL || strtable==NULL || strsz==0) {
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
    }
    if(sym==NULL || strtable==NULL || strsz==0)
        return nosym;

    /* find the closest symbol to the address */
    s=sym;
    ptr=0;
    for(i=0;i<(strtable-(char*)sym)/syment;i++) {
        if(s->st_name > strsz) break;
        if((virt_t)s->st_value <= (virt_t)addr &&
           (virt_t)s->st_value > ptr &&
           *((char*)(strtable + (uint32_t)s->st_name))!=0)
        {
            last = strtable + (uint32_t)s->st_name;
#if DEBUG
            lastsym = (uint64_t)s->st_value + (uint64_t)ehdr;
#endif
            ptr = s->st_value;
        }
        s++;
    }
    return last!=NULL && last[0]!=0 ? (uchar*)last : (uchar*)nosym;
}

/**
 * Fill in GOT entries and relocations. Will switch to address space
 *  - tmpmap: current task's TCB mapped
 *  - lastpt: last paging table
 *  - relas: array of rela_t items alloceted with kalloc()
 */
bool_t elf_rtlink()
{
    uint64_t i, j, k, n = 0, vs=kstrlen((char*)&osver), *objptr;
    tcb_t *tcb = (tcb_t*)0;
    rela_t *rel = relas;
    Elf64_Ehdr *ehdr;
    Elf64_Phdr *phdr_l;
    Elf64_Dyn *d;
    Elf64_Sym *sym=NULL, *s;
    Elf64_Rela *rela;
    Elf64_Rela *relad;
    virt_t vo;
    char *strtable;
    int strsz, syment, relasz, reladsz, relaent;

    /*** switch address space to make sure we flush changes in vmm tables ***/
    vmm_switch(((tcb_t*)&tmpmap)->memroot);
    /*** collect addresses to relocate ***/
    for(j=512; j<lastpt; j++) {
        ehdr=(Elf64_Ehdr *)(j*__PAGESIZE);
        if(ehdr==NULL || kmemcmp(ehdr->e_ident,ELFMAG,SELFMAG))
            continue;
        relasz=reladsz=0; relaent=24;
        /* set up instruction pointer for the first elf */
        // real entry point must be popped from stack and executed
        // after shared library's entry points all called
        vmm_setexec((virt_t)ehdr + ehdr->e_entry);
#if DEBUG
        if(debug&DBG_RTIMPORT)
            kprintf(" ELF %x%4D", ehdr, ehdr);
#endif
        /* get program headers. We have fixed segments: code, data, dynamic linkage */
        phdr_l=(Elf64_Phdr *)((uint8_t *)ehdr+ehdr->e_phoff+2*ehdr->e_phentsize);

        /* dynamic table */
        d = (Elf64_Dyn *)((uint8_t *)ehdr + phdr_l->p_offset);
#if DEBUG
        if(debug&DBG_RTIMPORT)
            kprintf("  Import %x (%d bytes):\n", d, (int)phdr_l->p_memsz);
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
                default:
                    break;
            }
            /* move pointer to next dynamic entry */
            d++;
        }
        // relocation and GOT entries
        /* GOT data entries */
        for(i = 0; i < reladsz / relaent; i++){
            kentropy();
            // failsafe
            if(n >= 2*__PAGESIZE/sizeof(rela_t))
                break;
            vo = (virt_t)((uint8_t*)ehdr + (int64_t)relad->r_offset);
            s = (Elf64_Sym *)((char *)sym + ELF64_R_SYM(relad->r_info) * syment);
            if(s!=NULL && *(strtable + s->st_name)!=0) {
                /* get the physical address and sym from stringtable */
                rel->offs = vo;
                rel->sym = strtable + s->st_name;
#if DEBUG
                if(debug&DBG_RTIMPORT)
                    kprintf("    %x D %s\n", vo, strtable + s->st_name);
#endif
                n++; rel++;
            } else {
#if DEBUG
                if(debug&DBG_RTIMPORT)
                    kprintf("    %x D base+%x\n", vo, *((uint64_t*)vo));
#endif
                *((uint64_t*)vo) += j*__PAGESIZE;
                //*((uint64_t*)vo) = j*__PAGESIZE + relad->r_addend;
            }
            /* move pointer to next rela entry */
            relad = (Elf64_Rela *)((uint8_t *)relad + relaent);
        }
        /* GOT plt entries */
        for(i = 0; i < relasz / relaent; i++){
            // failsafe
            if(n >= 2*__PAGESIZE/sizeof(rela_t))
                break;
            s = (Elf64_Sym *)((char *)sym + ELF64_R_SYM(rela->r_info) * syment);
            vo = (virt_t)((uint8_t*)ehdr + (int64_t)rela->r_offset);
            if(s!=NULL && *(strtable + s->st_name)!=0) {
                rel->offs = vo;
                rel->sym = strtable + s->st_name;
#if DEBUG
                if(debug&DBG_RTIMPORT)
                    kprintf("    %x T %s\n", rel->offs, strtable + s->st_name);
#endif
                n++; rel++;
            } else {
#if DEBUG
                if(debug&DBG_RTIMPORT)
                    kprintf("    %x T base+%x\n", vo, *((uint64_t*)vo));
#endif
                *((uint64_t*)vo) += j*__PAGESIZE;
                //*((uint64_t*)vo) = j*__PAGESIZE + relad->r_addend;
            }
            /* move pointer to next rela entry */
            rela = (Elf64_Rela *)((uint8_t *)rela + relaent);
        }
    }

    /*** resolve addresses ***/
    for(j=512; j<lastpt; j++) {
        ehdr=(Elf64_Ehdr *)(j*__PAGESIZE);
        if(ehdr==NULL || kmemcmp(ehdr->e_ident,ELFMAG,SELFMAG))
            continue;
        // place entry points onto the stack in reverse order
        vmm_pushentry((virt_t)ehdr + ehdr->e_entry);
        phdr_l=(Elf64_Phdr *)((uint8_t *)ehdr+ehdr->e_phoff+2*ehdr->e_phentsize);
        d = (Elf64_Dyn *)((uint8_t *)ehdr + phdr_l->p_offset);
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
        /* dynamic symbol table */
#if DEBUG
        if(debug&DBG_RTEXPORT)
            kprintf("  Export %x (%d bytes):\n", sym, strtable-(char*)sym);
#endif
        //foreach(sym)
        s=sym;
        for(i=0;i<(strtable-(char*)sym)/syment;i++) {
            kentropy();
            if(s->st_name > strsz) break;
            // is it defined in this object?
            if(s->st_value) {
                vo = (virt_t)((uint8_t*)ehdr + (uint64_t)s->st_value);
#if DEBUG
                if(debug&DBG_RTEXPORT)
                    kprintf("    %x %s:", vo, strtable + s->st_name);
#endif
                // parse irqX() symbols to fill up irq_routing_table
                if(tcb->priority == PRI_DRV) {
                    if(!kmemcmp(strtable + s->st_name,"irq",3)) {
                        k=0xFFFF;
                        //function, and at least one number
                        if(ELF64_ST_TYPE(s->st_info)==STT_FUNC &&
                            *(strtable + s->st_name + 3)>='0' && *(strtable + s->st_name + 3)<='9' &&
                            //strlen() = 4 | 5 | 6
                            (*(strtable + s->st_name + 4)==0 ||
                            *(strtable + s->st_name + 5)==0 ||
                            *(strtable + s->st_name + 6)==0)
                        ) {
                            // irq number from function name
                            env_dec((unsigned char*)strtable + s->st_name + 3, (uint*)&k, 0, 255-32);
                        } else {
                            if(ELF64_ST_TYPE(s->st_info)==STT_FUNC &&
                                *(strtable + s->st_name + 3)==0 && ehdr->e_version!=0xFFFF) {
                                // get autodetected irq number
                                k=0; *((uint16_t*)&k) = (uint16_t)ehdr->e_version;
                            } else if(ELF64_ST_TYPE(s->st_info)==STT_OBJECT &&
                                      ELF64_ST_BIND(s->st_info)==STB_GLOBAL &&
                                      ELF64_ST_VISIBILITY(s->st_other)==STV_DEFAULT &&
                                      !kmemcmp(strtable + s->st_name,"irqnum",7)) {
                                // if it's a variable with IRQ number
                                k = *((uint16_t*)((char*)ehdr + s->st_value));
                            }
                        }
                        if(k!=0xFFFF)
                            sys_installirq(k, tcb->memroot);
                    }
                }
                if(ELF64_ST_TYPE(s->st_info)==STT_OBJECT &&
                   ELF64_ST_BIND(s->st_info)==STB_GLOBAL &&
                   ELF64_ST_VISIBILITY(s->st_other)==STV_DEFAULT) {
                    objptr = (uint64_t*)vo;
                    // export data to drivers and services
                    if(tcb->priority == PRI_DRV || tcb->priority == PRI_SRV) {
                        if(!kmemcmp(strtable + s->st_name,"_bogomips",10) && s->st_size==8)
                            {*objptr=bogomips;} else
                        if(!kmemcmp(strtable + s->st_name,"_alarmstep",11) && s->st_size==8)
                            {*objptr=alarmstep;} else
                        if(!kmemcmp(strtable + s->st_name,"_mmio_ptr",10) && s->st_size==8)
                            {*objptr=mmio_ptr;} else
                        if(!kmemcmp(strtable + s->st_name,"_initrd_ptr",12) && s->st_size==8)
                            {*objptr=BUF_ADDRESS;} else
                        if(!kmemcmp(strtable + s->st_name,"_initrd_size",13) && s->st_size==8)
                            {*objptr=bootboot.initrd_size;} else
                        if(!kmemcmp(strtable + s->st_name,"_fstab_ptr",11) && s->st_size==8)
                            {*objptr=(fstab-INITRD_ADDRESS+BUF_ADDRESS);} else
                        if(!kmemcmp(strtable + s->st_name,"_fstab_size",12) && s->st_size==8)
                            {*objptr=fstab_size;} else
                        if(!kmemcmp(strtable + s->st_name,"_fb_ptr",8) && s->st_size==8)
                            {*objptr=(virt_t)BUF_ADDRESS;scrptr=true;} else
                        if(!kmemcmp(strtable + s->st_name,"_fb_width",10) && s->st_size>=4)
                            {kmemcpy(objptr,&bootboot.fb_width,4);} else
                        if(!kmemcmp(strtable + s->st_name,"_fb_height",11) && s->st_size>=4)
                            {kmemcpy(objptr,&bootboot.fb_height,4);} else
                        if(!kmemcmp(strtable + s->st_name,"_fb_scanline",13) && s->st_size>=4)
                            {kmemcpy(objptr,&bootboot.fb_scanline,4);} else
                        if(!kmemcmp(strtable + s->st_name,"_fb_type",9) && s->st_size>0)
                            {kmemcpy(objptr,&bootboot.fb_type,1);} else
                        if(!kmemcmp(strtable + s->st_name,"_display_type",14) && s->st_size>0)
                            {kmemcpy(objptr,&display,1);} else
                        if(!kmemcmp(strtable + s->st_name,"_syslog_ptr",12) && s->st_size==8)
                            {*objptr=buffer_ptr;} else
                        if(!kmemcmp(strtable + s->st_name,"_syslog_size",13) && s->st_size==8)
                            {*objptr=nrlogmax*__PAGESIZE;} else
                        if(!kmemcmp(strtable + s->st_name,"_environment",13) && s->st_size==8)
                            {*objptr=env_ptr;}
                    }
                    // export data to user processes (regardless to priority)
                    if(!kmemcmp(strtable + s->st_name,"_debug",7) && s->st_size>=4)
                        {kmemcpy(objptr,&debug,4);} else
                    if(!kmemcmp(strtable + s->st_name,"_osver",7) && s->st_size > vs) {
                        kmemcpy(objptr,&osver,vs+1);
                    }
//kprintf("obj ref: %x %d %x %x %s\n",objptr,s->st_size,s->st_value,*objptr,strtable + s->st_name);
                }
                // look up in relas array which addresses require
                // this symbol's virtual address
                for(k=0;k<n;k++){
                    rela_t *r = (rela_t*)((char *)relas + k*sizeof(rela_t));
                    if(r->offs != 0 && !kmemcmp(r->sym, strtable + s->st_name, kstrlen(r->sym)+1)) {
#if DEBUG
                        if(debug&DBG_RTEXPORT)
                            kprintf(" %x", r->offs);
#endif
                        // save virtual address
                        *((uint64_t *)r->offs) = vo;
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
        rela_t *r = (rela_t*)((char *)relas + i*sizeof(rela_t));
        if(r->offs!=0) {
            if(idle_first)
                kpanic("pid %x: shared library missing for '%s'", tcb->pid, r->sym);
            else {
                coreerrno=ENOEXEC;
                return false;
            }
        }
    }

    // sanity check task
    return vmm_check();
}

