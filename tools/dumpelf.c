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
    Elf64_Shdr *shdr=(Elf64_Shdr *)((uint8_t *)ehdr+ehdr->e_shoff);
    // string table and other section header entries
    Elf64_Shdr *strt=(Elf64_Shdr *)((uint8_t *)ehdr+ehdr->e_shoff+
        ehdr->e_shstrndx*ehdr->e_shentsize);
    Elf64_Shdr *rela_sh;
    Elf64_Shdr *dyn_sh;
    Elf64_Shdr *got_sh;
    Elf64_Shdr *dynsym_sh;
    Elf64_Shdr *dynstr_sh;
    // the string table
    char *strtable = elf + strt->sh_offset;
    int i;

    /* Section header */
    for(i = 0; i < ehdr->e_shnum; i++){
        if(!strcmp(strtable+shdr->sh_name, ".rela.plt")){
            rela_sh = shdr;
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
        if(!strcmp(strtable+shdr->sh_name, ".dynstr")){
            dynstr_sh = shdr;
        }
        /* move pointer to next section header */
        shdr = (Elf64_Shdr *)((uint8_t *)shdr + ehdr->e_shentsize);
    }

    /* dynstr table */
    printf("Stringtable %08lx (%d bytes), symbols %08lx (%d bytes, one entry %d)\n\n",
        dynstr_sh->sh_offset, (int)dynstr_sh->sh_size,
        dynsym_sh->sh_offset, (int)dynsym_sh->sh_size, (int)dynsym_sh->sh_entsize
    );

    printf("--- IMPORT ---\n");

    /* dynamic table */
    Elf64_Dyn *dyn = (Elf64_Dyn *)(elf + dyn_sh->sh_offset);
    printf("Dynamic %08lx (%d bytes, one entry %d):\n",
        dyn_sh->sh_offset, (int)dyn_sh->sh_size, (int)dyn_sh->sh_entsize
    );
    for(i = 0; i < dyn_sh->sh_size / dyn_sh->sh_entsize; i++){
        /* is it a needed record? */
        if(dyn->d_tag == DT_NEEDED) {
            printf("%3d. /lib/%s\n", i,
                (elf + dynstr_sh->sh_offset + dyn->d_un.d_ptr)
            );
        }
        /* move pointer to next dynamic entry */
        dyn = (Elf64_Dyn *)((uint8_t *)dyn + dyn_sh->sh_entsize);
    }
    
    /* GOT plt entries */
    Elf64_Rela *rela=(Elf64_Rela *)(elf + rela_sh->sh_offset);
    printf("\nGOT %08lx (%d bytes), Rela %08lx (%d bytes, one entry %d):\n",
        got_sh->sh_offset, (int)got_sh->sh_size,
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

    printf("\n--- EXPORT ---\n");

    Elf64_Sym *dynsym = (Elf64_Sym *)(elf + dynsym_sh->sh_offset);
    for(i = 0; i < dynsym_sh->sh_size / dynsym_sh->sh_entsize; i++){
        /* is it defined in this object? */
        if(dynsym->st_value && dynsym->st_size) {
            printf("%3d. %08lx %s\n", i, dynsym->st_value,
                (elf + dynstr_sh->sh_offset + dynsym->st_name)
            );
        }
        /* move pointer to next dynamic entry */
        dynsym = (Elf64_Sym *)((uint8_t *)dynsym + dynsym_sh->sh_entsize);
    }

}
