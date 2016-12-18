/*
 * tools/dumpelf.c
 * 
 * Copyright 2016 Public Domain bztsrc@github
 *
 * @brief Small utility to dump dynamic linking info in ELF64 objects
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

/* entry point */
int main(int argc,char** argv)
{
    /* if we have an argument, use that, otherwise ourself */
    char *elf = readfileall(argv[argc>1?1:0]);
    /* pointers to different elf structures */
    // elf file
    Elf64_Ehdr *ehdr=(Elf64_Ehdr *)(elf);
    // section header
    Elf64_Phdr *phdr=(Elf64_Phdr *)((uint8_t *)ehdr+ehdr->e_phoff);
    Elf64_Dyn *dyn;
    uint64_t got = 0;
    Elf64_Sym *sym, *s;
    Elf64_Rela *rela;
    // the string table
    char *strtable;
    int i, strsz, relasz, relaent, syment;

    /* Program header */
    for(i = 0; i < ehdr->e_phnum; i++){
        if(phdr->p_type==PT_DYNAMIC) {
            Elf64_Dyn *d;
            dyn = d = (Elf64_Dyn *)(elf + phdr->p_offset);
            while(d->d_tag != DT_NULL) {
                if(d->d_tag == DT_STRTAB) {
                    strtable = elf + (d->d_un.d_ptr&0xFFFF);
                }
                if(d->d_tag == DT_STRSZ) {
                    strsz = d->d_un.d_val;
                }
                /* move pointer to next dynamic entry */
                d++;
            }
            printf("Stringtable %08lx (%d bytes)\n\n", strtable - elf, strsz);
            printf("--- IMPORT ---\n");
            /* dynamic table */
            printf("Dynamic %08lx (%d bytes):\n",
                phdr->p_offset, (int)phdr->p_memsz
            );
            d = dyn;
            while(d->d_tag != DT_NULL) {
                if(d->d_tag == DT_SYMTAB) {
                    sym = (Elf64_Sym *)(elf + (d->d_un.d_ptr&0xFFFF));
                }
                if(d->d_tag == DT_SYMENT) {
                    syment = d->d_un.d_val;
                }
                if(d->d_tag == DT_JMPREL) {
                    rela = (Elf64_Rela *)(elf + (d->d_un.d_ptr&0xFFFF));
                }
                if(d->d_tag == DT_PLTRELSZ) {
                    relasz = d->d_un.d_val;
                }
                if(d->d_tag == DT_RELAENT) {
                    relaent = d->d_un.d_val;
                }
                if(d->d_tag == DT_PLTGOT) {
                    got = (d->d_un.d_ptr&0xFFFF);
                }
                if(d->d_tag == DT_NEEDED) {
                    printf("%3d. /lib/%s\n", i,
                        (strtable + d->d_un.d_ptr)
                    );
                }
                /* move pointer to next dynamic entry */
                d++;
            }
            break;
        }
        /* move pointer to next section header */
        phdr = (Elf64_Phdr *)((uint8_t *)phdr + ehdr->e_phentsize);
    }

    if(got) {
        /* GOT plt entries */
        printf("\nGOT %08lx, Rela %08lx (%d bytes, one entry %d):\n",
            got, ((char *)rela)-elf, relasz, relaent
        );
        for(i = 0; i < relasz / relaent; i++){
            s = (Elf64_Sym *)((char *)sym + ELF64_R_SYM(rela->r_info) * syment);
            /* get the string from stringtable for sym */
            printf("%3d. %08lx +%lx %s\n", i, rela->r_offset,
                rela->r_addend, strtable + s->st_name
            );
            /* move pointer to next rela entry */
            rela = (Elf64_Rela *)((uint8_t *)rela + relaent);
        }
    }

    printf("\n--- EXPORT ---\n");

    for(i=0;i<512;i++) {
        s = (Elf64_Sym *)((char *)sym + i * syment);
        if(s >= strtable || s->st_name > strsz) break;
        /* is it defined in this object? */
        if(s->st_value && s->st_size) {
            printf("%3d. %08lx %s\n", i, s->st_value, strtable + s->st_name);
        }
    }

}
