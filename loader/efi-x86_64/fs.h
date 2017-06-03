/*
 * loader/efi-x86_64/fs.h
 * 
 * Copyright 2017 Public Domain BOOTBOOT bztsrc@github
 * 
 * This file is part of the BOOTBOOT Protocol package.
 * @brief Filesystem drivers for initial ramdisk.
 * 
 */

#ifdef _FS_Z_H_
/**
 * FS/Z initrd (OS/Z's native file system)
 */
unsigned char *fsz_initrd(unsigned char *initrd_p, char *kernel)
{
    FSZ_SuperBlock *sb = (FSZ_SuperBlock *)initrd_p;
    FSZ_DirEnt *ent;
    FSZ_Inode *in=(FSZ_Inode *)(initrd_p+sb->rootdirfid*FSZ_SECSIZE);
    if(initrd_p==NULL || CompareMem(sb->magic,FSZ_MAGIC,4) || kernel==NULL){
        return NULL;
    }
    // Make sure only files in lib/ will be loaded
    CopyMem(kernel,"lib/",4);
    DBG(L" * FS/Z rootdir inode %d @%lx\n",sb->rootdirfid,in);
    // Get the inode of lib/sys/core
    int i;
    char *s,*e;
    s=e=kernel;
    i=0;
again:
    while(*e!='/'&&*e!=0){e++;}
    if(*e=='/'){e++;}
    if(!CompareMem(in->magic,FSZ_IN_MAGIC,4)){
        //is it inlined?
        if(!CompareMem(in->inlinedata,FSZ_DIR_MAGIC,4)){
            ent=(FSZ_DirEnt *)(in->inlinedata);
        } else if(!CompareMem(initrd_p+in->sec*FSZ_SECSIZE,FSZ_DIR_MAGIC,4)){
            // go, get the sector pointed
            ent=(FSZ_DirEnt *)(initrd_p+in->sec*FSZ_SECSIZE);
        } else {
            return NULL;
        }
        //skip header
        FSZ_DirEntHeader *hdr=(FSZ_DirEntHeader *)ent; ent++;
        //iterate on directory entries
        int j=hdr->numentries;
        while(j-->0){
            if(!CompareMem(ent->name,s,e-s)) {
                if(*e==0) {
                    i=ent->fid;
                    break;
                } else {
                    s=e;
                    in=(FSZ_Inode *)(initrd_p+ent->fid*FSZ_SECSIZE);
                    goto again;
                }
            }
            ent++;
        }
    } else {
        i=0;
    }
    DBG(L" * Kernel=%s inode %d @%lx\n",a2u(kernel),i,i?initrd_p+i*FSZ_SECSIZE:0);
    if(i!=0) {
        // fid -> inode ptr -> data ptr
        FSZ_Inode *in=(FSZ_Inode *)(initrd_p+i*FSZ_SECSIZE);
        if(!CompareMem(in->magic,FSZ_IN_MAGIC,4)){
            Elf64_Ehdr *ehdr=(Elf64_Ehdr *)(initrd_p + in->sec * FSZ_SECSIZE);
            if(!CompareMem(ehdr->e_ident,ELFMAG,SELFMAG))
                return (initrd_p + in->sec * FSZ_SECSIZE);
            else
                return (initrd_p + (unsigned int)(((FSZ_SectorList *)(initrd_p+in->sec*FSZ_SECSIZE))->fid) * FSZ_SECSIZE);
        }
    }
    return NULL;
}
#endif

/**
 * cpio hpodc archive
 */
unsigned char *cpio_initrd(unsigned char *initrd_p, char *kernel)
{
    unsigned char *ptr=initrd_p;
    int k;
    if(initrd_p==NULL || kernel==NULL || CompareMem(initrd_p,"070707",6))
        return NULL;
    DBG(L" * cpio %s\n",a2u(kernel));
    k=strlena((unsigned char*)kernel);
    while(!CompareMem(ptr,"070707",6)){
        int ns=oct2bin(ptr+8*6+11,6);
        int fs=oct2bin(ptr+8*6+11+6,11);
        if(!CompareMem(ptr+9*6+2*11,kernel,k+1)){
            return (unsigned char*)(ptr+9*6+2*11+ns);
        }
        ptr+=(76+ns+fs);
    }
    return NULL;
}

/**
 * ustar tarball archive
 */
unsigned char *tar_initrd(unsigned char *initrd_p, char *kernel)
{
    unsigned char *ptr=initrd_p;
    int k;
    if(initrd_p==NULL || kernel==NULL || CompareMem(initrd_p+257,"ustar",5))
        return NULL;
    DBG(L" * tar %s\n",a2u(kernel));
    k=strlena((unsigned char*)kernel);
    while(!CompareMem(ptr+257,"ustar",5)){
        int fs=oct2bin(ptr+0x7c,11);
        if(!CompareMem(ptr,kernel,k+1)){
            return (unsigned char*)(ptr+512);
        }
        ptr+=(((fs+511)/512)+1)*512;
    }
    return NULL;
}

/**
 * Static file system drivers registry
 */
unsigned char* (*fsdrivers[]) (unsigned char *, char *) = {
#ifdef _FS_Z_H_
    fsz_initrd,
#endif
    cpio_initrd,
    tar_initrd,
    NULL
};
