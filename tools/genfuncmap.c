/*
 * tools/genfuncmap.c
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
    Elf64_Sym *sym, *s;
    // the string table
    char *strtable;
    int i, strsz, syment;

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
            /* dynamic table */
            d = dyn;
            while(d->d_tag != DT_NULL) {
                if(d->d_tag == DT_SYMTAB) {
                    sym = (Elf64_Sym *)(elf + (d->d_un.d_ptr&0xFFFF));
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

    for(i=0;i<512;i++) {
        s = (Elf64_Sym *)((char *)sym + i * syment);
        if((uint64_t)s >= (uint64_t)strtable || s->st_name > strsz) break;
        /* is it defined in this object? */
        if(s->st_value && s->st_size) {
            printf("#define SYS_%s\t(%3d)\n", strtable + s->st_name, i);
        }
    }

}
