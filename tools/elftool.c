/*
 * tools/elftool.c
 *
 * Copyright 2017 Public Domain bztsrc@github
 *
 * @brief Quick and dirty tool to dump dynamic linking info in ELF64 objects
 *
 */

#include <elf.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* helper function to read a file into memory */
char* readfileall(char *file)
{
    char *data=NULL;
    FILE *f;
    long int s=0;
    f=fopen(file,"r");
    if(f){
        fseek(f,0L,SEEK_END);
        s=ftell(f);
        fseek(f,0L,SEEK_SET);
        data=(char*)malloc(s+1);
        if(data==NULL) {
            printf("Unable to allocate %ld memory\n", s+1);
            exit(2);
        }
        fread(data, s, 1, f);
        fclose(f);
    } else {
        printf("File not found\n");
        exit(1);
    }
    return data;
}

unsigned long int hextoi(char *s)
{
    uint64_t v = 0;
    if(*s=='0' && *(s+1)=='x')
        s+=2;
    do{
        v <<= 4;
        if(*s>='0' && *s<='9')
            v += (uint64_t)((unsigned char)(*s)-'0');
        else if(*s >= 'a' && *s <= 'f')
            v += (uint64_t)((unsigned char)(*s)-'a'+10);
        else if(*s >= 'A' && *s <= 'F')
            v += (uint64_t)((unsigned char)(*s)-'A'+10);
        s++;
    } while((*s>='0'&&*s<='9')||(*s>='a'&&*s<='f')||(*s>='A'&&*s<='F'));
    return v;
}

/* entry point */
int main(int argc,char** argv)
{
    char *binds[] = { "L", "G", "W", "N" };
    char *types[] = { "NOT", "OBJ", "FNC", "SEC", "FLE", "COM", "TLS", "NUM", "LOS", "IFC", "HOS", "LPR", "HPR" };

    int i=0, dump=0, strsz, syment;
    unsigned long int reloc=-1;
    if(argc<2) {
        printf("./elftool [elf]                 dumps dynamic section into C header\n"
               "./elftool -d [elf]              dumps sections in human readable format\n"
               "./elftool -s [reloc] [elf]      dumps symbols with relocated addresses\n");
        return 1;
    }
    if(argc>2 && argv[1][0]=='-' && argv[1][1]=='s') {
        reloc=hextoi(argv[2]);
    } else if(argv[1][0]=='-' && argv[1][1]=='d') {
        dump=1;
    }
    /* if we have an argument, use that, otherwise ourself */
    char *elf = readfileall(argv[argc-1]);

    /* pointers to different elf structures */
    // elf file
    Elf64_Ehdr *ehdr=(Elf64_Ehdr *)(elf);

    // section header
    Elf64_Phdr *phdr=(Elf64_Phdr *)((uint8_t *)ehdr+ehdr->e_phoff);
    Elf64_Sym *sym = NULL, *s = NULL;
    // the string table
    char *strtable = NULL;

    /* Program header */
    if(dump)
        printf("Segments\n");
    for(i = 0; i < ehdr->e_phnum; i++){
        if(dump)
            printf(" %2d %08lx %6ld %s\n",(int)phdr->p_type,phdr->p_offset,phdr->p_filesz,
                (phdr->p_type==PT_LOAD?"load":(phdr->p_type==PT_DYNAMIC?"dynamic":"")));
        /* we only need the Dynamic header */
        if(phdr->p_type==PT_DYNAMIC) {
            /* read entries in Dynamic section */
            Elf64_Dyn *d;
            d = (Elf64_Dyn *)(elf + phdr->p_offset);
            while(d->d_tag != DT_NULL) {
                if(dump && d->d_tag<1000)
                    printf("     %3d %08lx %s\n",(int)d->d_tag,d->d_un.d_ptr,
                        (d->d_tag==DT_STRTAB?"strtab":(d->d_tag==DT_SYMTAB?"symtab":"")));
                if(d->d_tag == DT_STRTAB) {
                    strtable = elf + (d->d_un.d_ptr&0xFFFFFFFF);
                }
                if(d->d_tag == DT_STRSZ) {
                    strsz = d->d_un.d_val;
                }
                if(d->d_tag == DT_SYMTAB) {
                    sym = (Elf64_Sym *)(elf + (d->d_un.d_ptr&0xFFFFFFFF));
                }
                if(d->d_tag == DT_SYMENT) {
                    syment = d->d_un.d_val;
                }
                /* move pointer to next dynamic entry */
                d++;
            }
            break;
        }
        /* move pointer to next section header */
        phdr = (Elf64_Phdr *)((uint8_t *)phdr + ehdr->e_phentsize);
    }

    // if dump of sections was requested on command line
    if(dump || reloc!=-1) {
        // section header
        Elf64_Shdr *shdr=(Elf64_Shdr *)((uint8_t *)ehdr+ehdr->e_shoff);
        // string table and other section header entries
        Elf64_Shdr *strt=(Elf64_Shdr *)((uint8_t *)shdr+(uint64_t)ehdr->e_shstrndx*(uint64_t)ehdr->e_shentsize);
        Elf64_Shdr *rela_sh=NULL;
        Elf64_Shdr *relat_sh=NULL;
        Elf64_Shdr *dyn_sh=NULL;
        Elf64_Shdr *got_sh=NULL;
        Elf64_Shdr *dynsym_sh=NULL;
        Elf64_Shdr *dynstr_sh=NULL;
        Elf64_Shdr *sym_sh=NULL;
        Elf64_Shdr *str_sh=NULL;
        // the string table
        strtable = elf + strt->sh_offset;
        int i;

        if(dump)
            printf("\nPhdr %lx Shdr %lx Strt %lx Sstr %lx\n\nSections\n",
                (uint64_t)ehdr->e_phoff,
                (uint64_t)ehdr->e_shoff,
                (uint64_t)strt-(uint64_t)ehdr,
                (uint64_t)strt->sh_offset
            );

        /* Section header */
        for(i = 0; i < ehdr->e_shnum; i++){
            if(dump)
                printf(" %2d %08lx %6ld %s\n",i,
                    (uint64_t)shdr->sh_offset, (uint64_t)shdr->sh_size,
                    strtable+shdr->sh_name);
            if(!strcmp(strtable+shdr->sh_name, ".rela.plt")){
                rela_sh = shdr;
            }
            if(!strcmp(strtable+shdr->sh_name, ".rela.text")||
               !strcmp(strtable+shdr->sh_name, ".rela.dyn")){
                relat_sh = shdr;
            }
            if(!strcmp(strtable+shdr->sh_name, ".dynamic")){
                dyn_sh = shdr;
            }
            if(!strcmp(strtable+shdr->sh_name, ".got.plt")){
                got_sh = shdr;
            }
            if(!strcmp(strtable+shdr->sh_name, ".dynsym")){
                dynsym_sh = shdr;
            }
            if(!strcmp(strtable+shdr->sh_name, ".symtab")){
                sym_sh = shdr;
            }
            if(!strcmp(strtable+shdr->sh_name, ".dynstr")){
                dynstr_sh = shdr;
            }
            if(!strcmp(strtable+shdr->sh_name, ".strtab")){
                str_sh = shdr;
            }
            /* move pointer to next section header */
            shdr = (Elf64_Shdr *)((uint8_t *)shdr + ehdr->e_shentsize);
        }
        if(dump || reloc!=-1) {
            if(str_sh!=NULL) {
                strtable = elf + str_sh->sh_offset;
                strsz = (int)str_sh->sh_size;
            }
            if(sym_sh!=NULL) {
                sym = (Elf64_Sym *)(elf + (sym_sh->sh_offset));
                syment = (int)sym_sh->sh_entsize;
            }
        }
        if(!dump) {
            goto output;
        }
        if(dynstr_sh==NULL)
            dynstr_sh=str_sh;
        if(dynsym_sh==NULL)
            dynsym_sh=sym_sh;

        /* dynstr table */
        printf("\nStringtable %08lx (%d bytes), symbols %08lx (%d bytes, one entry %d)\n\n",
            dynstr_sh->sh_offset, (int)dynstr_sh->sh_size,
            dynsym_sh->sh_offset, (int)dynsym_sh->sh_size, (int)dynsym_sh->sh_entsize
        );

        printf("--- IMPORT ---\n");

        /* dynamic table */
        if(dyn_sh!=NULL) {
            Elf64_Dyn *dyn = (Elf64_Dyn *)(elf + dyn_sh->sh_offset);
            printf("Dynamic %08lx (%d bytes, one entry %d):\n",
                dyn_sh->sh_offset, (int)dyn_sh->sh_size, (int)dyn_sh->sh_entsize
            );
            for(i = 0; i < dyn_sh->sh_size / dyn_sh->sh_entsize; i++){
                /* is it a needed record? */
                if(dyn->d_tag == DT_NEEDED) {
                    printf("%3d. /lib/%s\n", i,
                        ((char*)elf + (uint)dynstr_sh->sh_offset + (uint)dyn->d_un.d_ptr)
                    );
                }
                /* move pointer to next dynamic entry */
                dyn = (Elf64_Dyn *)((uint8_t *)dyn + dyn_sh->sh_entsize);
            }

            /* GOT plt entries */
            if(got_sh)
                printf("\nGOT %08lx (%d bytes)",
                    got_sh->sh_offset, (int)got_sh->sh_size
                );
            if(rela_sh) {
                Elf64_Rela *rela=(Elf64_Rela *)(elf + rela_sh->sh_offset);
                printf(", Rela plt %08lx (%d bytes, one entry %d):\n",
                    rela_sh->sh_offset, (int)rela_sh->sh_size, (int)rela_sh->sh_entsize
                );

                for(i = 0; i < rela_sh->sh_size / rela_sh->sh_entsize; i++){
                    /* get symbol, this is a bit terrible, but basicly offset calculation */
                    Elf64_Sym *sym = (Elf64_Sym *)(
                        elf + dynsym_sh->sh_offset +                        //base address
                        ELF64_R_SYM(rela->r_info) * dynsym_sh->sh_entsize   //offset
                    );
                    /* get the string from stringtable for sym */
                    char *symbol = (elf + dynstr_sh->sh_offset + sym->st_name);
                    printf("%3d. %08lx +%lx %s\n", i, rela->r_offset,
                        rela->r_addend, symbol
                    );
                    /* move pointer to next rela entry */
                    rela = (Elf64_Rela *)((uint8_t *)rela + rela_sh->sh_entsize);
                }
            }
            if(relat_sh) {
                Elf64_Rela *rela=(Elf64_Rela *)(elf + relat_sh->sh_offset);
                printf("\nRela text %08lx (%d bytes, one entry %d):\n",
                    rela_sh->sh_offset, (int)rela_sh->sh_size, (int)rela_sh->sh_entsize
                );

                for(i = 0; i < relat_sh->sh_size / relat_sh->sh_entsize; i++){
                    /* get symbol, this is a bit terrible, but basicly offset calculation */
                    Elf64_Sym *sym = (Elf64_Sym *)(
                        elf + dynsym_sh->sh_offset +                        //base address
                        ELF64_R_SYM(rela->r_info) * dynsym_sh->sh_entsize   //offset
                    );
                    /* get the string from stringtable for sym */
                    char *symbol = (elf + dynstr_sh->sh_offset + sym->st_name);
                    printf("%3d. %08lx +%lx %s\n", i, rela->r_offset,
                        rela->r_addend, symbol
                    );
                    /* move pointer to next rela entry */
                    rela = (Elf64_Rela *)((uint8_t *)rela + relat_sh->sh_entsize);
                }
            }
        }
        printf("\n--- EXPORT ---\n");

        s = sym;
        for(i=0;i<(strtable-(char*)sym)/syment;i++) {
            if(s->st_name > strsz) break;
            printf("%3d. %08lx %x %s %x %s %s\n", i, s->st_value, 
                ELF64_ST_BIND(s->st_info), binds[ELF64_ST_BIND(s->st_info)],
                ELF64_ST_TYPE(s->st_info), types[ELF64_ST_TYPE(s->st_info)],
                strtable + s->st_name);
            s++;
        }
        return 0;
    }

output:
    if(strtable==NULL || sym==NULL) {
        printf("no symtab or strtab?\n");
        return 1;
    }

    if(reloc==-1) {
        char *name = strrchr(argv[argc-1], '/');
        if(name!=NULL && name[0]=='/') name++;
        printf("/*\n"
            " * sys/%s.h - GENERATED BY elftool DO NOT EDIT THIS FILE\n"
            " *\n"
            " * Copyright 2017 CC-by-nc-sa bztsrc@github\n"
            " * https://creativecommons.org/licenses/by-nc-sa/4.0/\n"
            " * \n"
            " * You are free to:\n"
            " *\n"
            " * - Share — copy and redistribute the material in any medium or format\n"
            " * - Adapt — remix, transform, and build upon the material\n"
            " *     The licensor cannot revoke these freedoms as long as you follow\n"
            " *     the license terms.\n"
            " *\n"
            " * Under the following terms:\n"
            " *\n"
            " * - Attribution — You must give appropriate credit, provide a link to\n"
            " *     the license, and indicate if changes were made. You may do so in\n"
            " *     any reasonable manner, but not in any way that suggests the\n"
            " *     licensor endorses you or your use.\n"
            " * - NonCommercial — You may not use the material for commercial purposes.\n"
            " * - ShareAlike — If you remix, transform, or build upon the material,\n"
            " *     you must distribute your contributions under the same license as\n"
            " *     the original.\n"
            " *\n"
            " * @brief OS/Z system calls for %s service. Include with osZ.h\n"
            " */\n\n", name, name);
    }

    s = sym;
    for(i=0;i<(strtable-(char*)sym)/syment;i++) {
        if(s->st_name > strsz) break;
        /* is it defined in this object and not binary import? */
        if( s->st_value && *(strtable + s->st_name)!=0 && *(strtable + s->st_name)!='_' ) {
            if(reloc!=-1)
                printf("%08lx %s\n", s->st_value+reloc, strtable + s->st_name);
            else
            /* don't include entry point... GNU ld can't set private entry points */
            if(ELF64_ST_TYPE(s->st_info)==STT_FUNC && s->st_size && 
                ELF64_ST_BIND(s->st_info)==STB_GLOBAL && ELF64_ST_VISIBILITY(s->st_other)==STV_DEFAULT && 
                strcmp(strtable + s->st_name, "mq_dispatch") && strcmp(strtable + s->st_name, "main"))
                /* +3: make sure not conflicting with IRQ, ack and nack */
                printf("#define SYS_%s\t%s(0x%02x)\n",
                    strtable + s->st_name, strlen(strtable + s->st_name)<4?"\t":"", i+3);
        }
        s++;
    }
    if(reloc==-1)
        printf("\n");
}
