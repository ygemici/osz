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
 * @brief Early ELF64 loader and parser, used by service.c. Relies on identity mapping.
 */

#include <elf.h>
#include "env.h"

/* external resources */
extern char osver[];
extern uint8_t _code;
extern uint64_t *stack_ptr;
extern uint8_t sys_fault;
extern uint8_t scrptr;
extern uint64_t fstab;
extern uint64_t fstab_size;
extern uint64_t alarmstep;
extern uint64_t bogomips;

extern unsigned char *env_dec(unsigned char *s, uint *v, uint min, uint max);

/* we define the addresses as 32 bit, as code segment cannot grow
   above 4G. This way we can have 512 relocation records in a page */

/* memory allocated for relocation addresses */
dataseg rela_t *relas;

/* dynsym */
dataseg unsigned char *nosym = (unsigned char*)"(no symbol)";
dataseg unsigned char *mqsym = (unsigned char*)"(mq)";
dataseg unsigned char *lssym = (unsigned char*)"(local stack)";
#if DEBUG
extern ccb_t ccb;
dataseg virt_t lastsym;
#endif
/* page aligned elfs */
typedef struct {
    Elf64_Ehdr *ptr;
    char name[56];
} elfcache_t;
dataseg elfcache_t *alignedelf = NULL;

/**
 * load an ELF64 binary into text segment starting at 2M
 */
void *elf_load(char *fn)
{
    // failsafe
    if(fn==NULL)
        return NULL;
    // this is so early, we don't have initrd in fs task' bss yet.
    // so we have to rely on identity mapping to locate the files
    tcb_t *tcb = (tcb_t*)(pmm.bss_end);
    uint64_t *paging = (uint64_t *)&tmpmap;
    /* locate elf on initrd */
    Elf64_Ehdr *elf=NULL;
    elfcache_t *ec=alignedelf;
    if(alignedelf!=NULL) { while(ec->ptr) { if(!kstrcmp(ec->name,fn)) { elf=ec->ptr; break; } ec++; } }
    if(elf==NULL) {
        elf=(Elf64_Ehdr *)fs_locate(fn);
        if(((virt_t)elf)&(__PAGESIZE-1)) {
            if(alignedelf==NULL) {
                alignedelf=kalloc(1);
                kmemset(alignedelf,0,__PAGESIZE);
            }
            ec=alignedelf;
            while(ec->ptr) { if(!kstrcmp(ec->name,fn)) { elf=ec->ptr; break; } ec++; }
            ec->ptr=(Elf64_Ehdr *)pmm_alloc((fs_size+__PAGESIZE-1)/__PAGESIZE);
            kmemcpy(ec->ptr,elf,fs_size);
            elf=ec->ptr;
        }
    }
    /* get program headers */
    Elf64_Phdr *phdr_c=(Elf64_Phdr *)((uint8_t *)elf+elf->e_phoff);
    Elf64_Phdr *phdr_d=(Elf64_Phdr *)((uint8_t *)elf+elf->e_phoff+1*elf->e_phentsize);
    Elf64_Phdr *phdr_l=(Elf64_Phdr *)((uint8_t *)elf+elf->e_phoff+2*elf->e_phentsize);
    int i=0,ret,size=(fs_size+__PAGESIZE-1)/__PAGESIZE;
    // PT at tmpmap
    while((paging[i]&1)!=0 && i<1024-size) i++;
    if((paging[i]&1)!=0) {
        kpanic("out of memory, text segment too big: %s", fn);
    }
    ret = i;
    srand[(i+0)%4] *= 16807;
    srand[(i+1)%4] ^= (uint64_t)elf;
    srand[(i+2)%4] *= 16807;
    srand[(i+3)%4] ^= (uint64_t)elf;

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
            kprintf("WARNING corrupt ELF binary: %s\n", fn);
            return (void*)(-1);
    }
    /* clear autodetected irq number field */
    elf->e_machine = 255;
    /* map text segment */
    size=(phdr_c->p_filesz+__PAGESIZE-1)/__PAGESIZE;
    while(size--) {
        paging[i]=(uint64_t)((char*)elf + (i-ret)*__PAGESIZE + PG_USER_RO);
        fs_size -= __PAGESIZE;
        tcb->linkmem++;
        i++;
    }
    // failsafe
    if(fs_size<0) fs_size=0;

    /* allocate data segment, page aligned */
    size=(fs_size+__PAGESIZE-1)/__PAGESIZE;
    /* GNU ld bug workaround: keep symtab and strtab */
    size++;
    while(size--) {
        void *pm = pmm_alloc(1);
        kmemcpy(pm,(char *)elf + (i-ret)*__PAGESIZE,__PAGESIZE);
        paging[i]=((uint64_t)(pm) + PG_USER_RW)|((uint64_t)1<<63);
        tcb->allocmem++;
        i++;
    }

    /* return start offset within text segment */
    return (void*)((uint64_t)ret * __PAGESIZE);
}

/**
 * load an ELF64 shared library
 */
__inline__ void elf_loadso(char *fn)
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
    uchar c = name[size];
    name[size]=0;
    /* common to all tasks */
    virt_t addr = TEXT_ADDRESS;
    if(size==3 && !kmemcmp(name,"tcb",4)) {
        name[size] = c;
        return 0;
    }
    if(size==2 && !kmemcmp(name,"mq",3)) {
        name[size] = c;
        return __PAGESIZE;
    }
    if(size==5 && !kmemcmp(name,"stack",6)) {
        name[size] = c;
        return __SLOTSIZE/2;
    }
    if(size==4 && !kmemcmp(name,"text",5)) {
        name[size] = c;
        return TEXT_ADDRESS;
    }
    if(size==3 && !kmemcmp(name,"bss",4)) {
        name[size] = c;
        return BSS_ADDRESS;
    }
    if(size==4 && !kmemcmp(name,"sbss",4)) {
        name[size] = c;
        return SBSS_ADDRESS;
    }
    if(size==6 && !kmemcmp(name,"shared",6)) {
        name[size] = c;
        return SBSS_ADDRESS;
    }
    if(size==4 && !kmemcmp(name,"core",5)) {
        name[size] = c;
        return CORE_ADDRESS;
    }
    if(size==4 && !kmemcmp(name,"buff",5)) {
        name[size] = c;
        return BUF_ADDRESS;
    }
    if(size==2 && !kmemcmp(name,"bt",3)) {
        name[size] = c;
        return (virt_t)(ccb.ist3+ISR_STACK-40);
    }
    name[size] = c;
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
 * load dynamic libraries. Call with 1 if libc already loaded
 */
void elf_neededso(int libc)
{
    int i, j;
    uint64_t *paging = (uint64_t *)&tmpmap;
    Elf64_Dyn *d;
    char *strtable;
    char lib[256]="sys/lib/";

    /*** collect addresses to relocate ***/
    for(j=0; j<__PAGESIZE/8; j++) {
        Elf64_Ehdr *ehdr=(Elf64_Ehdr *)(paging[j]&~(__PAGESIZE-1)&~((uint64_t)1<<63));
        if(ehdr==NULL || kmemcmp(ehdr->e_ident,ELFMAG,SELFMAG))
            continue;
        Elf64_Phdr *phdr=(Elf64_Phdr *)((uint8_t *)ehdr+ehdr->e_phoff);

        /* Program header */
        for(i = 0; i < ehdr->e_phnum; i++){
            if(phdr->p_type==PT_DYNAMIC) {
                /* dynamic table */
                d = (Elf64_Dyn *)((uint8_t *)ehdr + phdr->p_offset);
                while(d->d_tag != DT_NULL) {
                    if(d->d_tag==DT_STRTAB) {
                        strtable = (char *)ehdr + d->d_un.d_ptr;
                        break;
                    }
                    d++;
                }
                d = (Elf64_Dyn *)((uint8_t *)ehdr + phdr->p_offset);
                while(d->d_tag != DT_NULL) {
                    if(d->d_tag==DT_NEEDED) {
                        if(!kmemcmp(strtable + d->d_un.d_ptr, "libc.so", 8)) {
                            if(!libc)
                                elf_loadso("sys/lib/libc.so");
                            libc++;
                        } else {
                            kstrcpy((char*)&lib[4],strtable + d->d_un.d_ptr);
                            elf_loadso(lib);
                        }
                    }
                    /* move pointer to next dynamic entry */
                    d++;
                }
                break;
            }
            /* move pointer to next program header */
            phdr = (Elf64_Phdr *)((uint8_t *)phdr + ehdr->e_phentsize);
        }
    }
}

/**
 * Fill in GOT entries. Relies on identity mapping
 *  - tmpmap: the text segment's PT mapped,
 *  - pmm.bss_end: current task's TCB mapped,
 *  - relas: array of rela_t items alloceted with kalloc()
 *  - stack_ptr: physical address of task's stack
 */
bool_t elf_rtlink()
{
    int i, j, k, n = 0, vs=kstrlen((char*)&osver);
    tcb_t *tcb = (tcb_t*)(pmm.bss_end);
    rela_t *rel = relas;
    uint64_t *paging = (uint64_t *)&tmpmap, *objptr;

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
                kentropy();
                // failsafe
                if(n >= 2*__PAGESIZE/sizeof(rela_t))
                    break;
                s = (Elf64_Sym *)((char *)sym + ELF64_R_SYM(relad->r_info) * syment);
                if(s!=NULL && *(strtable + s->st_name)!=0) {
                    /* get the physical address and sym from stringtable */
                    uint64_t o = (uint64_t)((int64_t)relad->r_offset + j*__PAGESIZE + (int64_t)relad->r_addend);
                    /* because the task is not mapped yet, we have to translate
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
                /* get the physical address and sym from stringtable */
                uint64_t o = (uint64_t)((int64_t)rela->r_offset + j*__PAGESIZE + (int64_t)rela->r_addend);
                /* because the task is not mapped yet, we have to translate
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
            kprintf("  Export %x (%d bytes):\n", sym, strtable-(char*)sym);
#endif
        //foreach(sym)
        s=sym;
        for(i=0;i<(strtable-(char*)sym)/syment;i++) {
            kentropy();
            if(s->st_name > strsz) break;
            // is it defined in this object?
            if(s->st_value) {
                uint64_t offs = (uint64_t)s->st_value + j*__PAGESIZE+TEXT_ADDRESS;
#if DEBUG
                if(debug&DBG_RTEXPORT)
                    kprintf("    %x %s:", offs, strtable + s->st_name);
#endif
                // parse irqX() symbols to fill up irq_routing_table
                if(((tcb_t*)(pmm.bss_end))->priority == PRI_DRV) {
                    if(!kmemcmp(strtable + s->st_name,"irq",3)) {
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
                                *(strtable + s->st_name + 3)==0 && ehdr->e_machine!=255) {
                                // get autodetected irq number
                                k=0; *((uint8_t*)&k) = (uint8_t)ehdr->e_machine;
                            } else if(ELF64_ST_TYPE(s->st_info)==STT_OBJECT &&
                                      ELF64_ST_BIND(s->st_info)==STB_GLOBAL &&
                                      ELF64_ST_VISIBILITY(s->st_other)==STV_DEFAULT &&
                                      !kmemcmp(strtable + s->st_name,"irqnum",7)) {
                                // if it's a variable with IRQ number
                                k = *((uint64_t*)((char*)ehdr + s->st_value));
                            }
                        }
                        isr_installirq(k, ((tcb_t*)(pmm.bss_end))->memroot);
                    }
                }
                if(ELF64_ST_TYPE(s->st_info)==STT_OBJECT &&
                   ELF64_ST_BIND(s->st_info)==STB_GLOBAL &&
                   ELF64_ST_VISIBILITY(s->st_other)==STV_DEFAULT) {
                    objptr = (uint64_t*)((paging[j+s->st_value/__PAGESIZE]&~(__PAGESIZE-1)&~((uint64_t)1<<63)) + s->st_value%__PAGESIZE);
                    // export data to drivers and services
                    if( ((tcb_t*)(pmm.bss_end))->priority == PRI_DRV ||
                        ((tcb_t*)(pmm.bss_end))->priority == PRI_SRV) {
                        k=0;
                        if(!kmemcmp(strtable + s->st_name,"_bogomips",10) && s->st_size==8)
                            {k=8; *objptr=bogomips;}
                        if(!kmemcmp(strtable + s->st_name,"_alarmstep",11) && s->st_size==8)
                            {k=8; *objptr=alarmstep;}
                        if(!kmemcmp(strtable + s->st_name,"_initrd_ptr",12) && s->st_size==8)
                            {k=8; *objptr=BUF_ADDRESS;}
                        if(!kmemcmp(strtable + s->st_name,"_initrd_size",13) && s->st_size==8)
                            {k=8; *objptr=bootboot.initrd_size;}
                        if(!kmemcmp(strtable + s->st_name,"_fstab_ptr",11) && s->st_size==8)
                            {k=8; *objptr=(fstab-bootboot.initrd_ptr+BUF_ADDRESS);}
                        if(!kmemcmp(strtable + s->st_name,"_fstab_size",12) && s->st_size==8)
                            {k=8; *objptr=fstab_size;}
                        if(!kmemcmp(strtable + s->st_name,"_pathmax",9) && s->st_size>=4)
                            {k=4; kmemcpy(objptr,&pathmax,4);}
                        if(!kmemcmp(strtable + s->st_name,"_fb_width",10) && s->st_size>=4)
                            {k=4; kmemcpy(objptr,&bootboot.fb_width,4);}
                        if(!kmemcmp(strtable + s->st_name,"_fb_height",11) && s->st_size>=4)
                            {k=4; kmemcpy(objptr,&bootboot.fb_height,4);}
                        if(!kmemcmp(strtable + s->st_name,"_fb_scanline",13) && s->st_size>=4)
                            {k=4; kmemcpy(objptr,&bootboot.fb_scanline,4);}
                        if(!kmemcmp(strtable + s->st_name,"_fb_type",9) && s->st_size>0)
                            {k=1; kmemcpy(objptr,&bootboot.fb_type,1);}
                        if(!kmemcmp(strtable + s->st_name,"_display_type",14) && s->st_size>0)
                            {k=1; kmemcpy(objptr,&display,1);}
                        if(!kmemcmp(strtable + s->st_name,"_lefthanded",12) && s->st_size>0)
                            {k=1; kmemcpy(objptr,&lefthanded,1);}
                        if(!kmemcmp(strtable + s->st_name,"_debug",7) && s->st_size>0)
                            {k=4; kmemcpy(objptr,&debug,4);}
                        if(!kmemcmp(strtable + s->st_name,"_identity",10) && s->st_size>0)
                            {k=1; kmemcpy(objptr,&identity,1);}
                        if(!kmemcmp(strtable + s->st_name,"_rescueshell",13) && s->st_size>0)
                            {k=1; kmemcpy(objptr,&rescueshell,1);}
                        if(!kmemcmp(strtable + s->st_name,"_keymap",8) && s->st_size==8)
                            {k=8; kmemcpy(objptr,&keymap,8);}
                        if(!kmemcmp(strtable + s->st_name,"_screen_ptr",12) && s->st_size==8)
                            {k=8; *objptr=BUF_ADDRESS;scrptr=true;}
                        if(!kmemcmp(strtable + s->st_name,"_fb_ptr",8) && s->st_size==8)
                            {k=8; *objptr=(virt_t)BUF_ADDRESS + ((virt_t)__SLOTSIZE * ((virt_t)__PAGESIZE / 8));}
                        if(k && (s->st_value%__PAGESIZE)+k>__PAGESIZE)
                            syslog_early("pid %x: exporting %s on page boundary",
                                ((tcb_t*)(pmm.bss_end))->pid,strtable + s->st_name);
                    }
                    // export data to user processes
                    if(!kmemcmp(strtable + s->st_name,"_osver",7) && s->st_size > vs) {
                        if((s->st_value%__PAGESIZE)+vs>__PAGESIZE)
                            syslog_early("pid %x: exporting %s on page boundary",
                                ((tcb_t*)(pmm.bss_end))->pid,strtable + s->st_name);
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
                        uint64_t *ptr = (uint64_t *)r->offs;
                        // soecial cases, they're in the TCB
                        if(!kmemcmp(strtable + s->st_name, "errno", 6))
                            offs = tcb_errno;
                        if(!kmemcmp(strtable + s->st_name, "_rootdir", 9))
                            offs = tcb_rootdir;
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
        rela_t *r = (rela_t*)((char *)relas + i*sizeof(rela_t));
        if(r->offs!=0) {
            kpanic("pid %x: shared library missing for %s()", ((tcb_t*)(pmm.bss_end))->pid, r->sym);
        }
    }

    // push entry point as the last callback
    // crt0 starts with a 'ret', so _start function address
    // will be popped up and executed.
    Elf64_Ehdr *ehdr=(Elf64_Ehdr *)(paging[0]&~(__PAGESIZE-1)&~((uint64_t)1<<63));
    if(ehdr==NULL || kmemcmp(ehdr->e_ident,ELFMAG,SELFMAG) || ehdr->e_phoff==ehdr->e_entry)
        kpanic("no executor?");
    *stack_ptr = ehdr->e_entry + TEXT_ADDRESS;
    stack_ptr--;
    tcb->rsp -= 8;

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
    // sanity check task
    return task_check(tcb, (phy_t*)paging);
}

