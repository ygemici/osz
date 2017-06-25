/*
 * loader/efi-x86_64/fs.h
 * 
 * Copyright 2017 Public Domain BOOTBOOT bztsrc@github
 * 
 * This file is part of the BOOTBOOT Protocol package.
 * @brief Filesystem drivers for initial ramdisk.
 * 
 */

/**
 * return type for fs drivers
 */
typedef struct {
    UINT8 *ptr;
    UINTN size;
} file_t;

extern int oct2bin(unsigned char *str,int size);
extern int hex2bin(unsigned char *str,int size);
extern CHAR16 *a2u (char *str);

#ifdef _FS_Z_H_
/**
 * FS/Z initrd (OS/Z's native file system)
 */
file_t fsz_initrd(unsigned char *initrd_p, char *kernel)
{
    FSZ_SuperBlock *sb = (FSZ_SuperBlock *)initrd_p;
    FSZ_DirEnt *ent;
    FSZ_Inode *in=(FSZ_Inode *)(initrd_p+sb->rootdirfid*FSZ_SECSIZE);
    file_t ret = { NULL, 0 };
    if(initrd_p==NULL || CompareMem(sb->magic,FSZ_MAGIC,4) || kernel==NULL){
        return ret;
    }
    DBG(L" * FS/Z %s\n",a2u(kernel));
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
            return ret;
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
    if(i!=0) {
        // fid -> inode ptr -> data ptr
        FSZ_Inode *in=(FSZ_Inode *)(initrd_p+i*FSZ_SECSIZE);
        if(!CompareMem(in->magic,FSZ_IN_MAGIC,4)){
            ret.size=in->size;
            //inline
            if(in->size<FSZ_SECSIZE-1024) {
                ret.ptr=(UINT8*)(initrd_p+i*FSZ_SECSIZE+1024);
            }
            //direct
            if(in->size<FSZ_SECSIZE) {
                ret.ptr=(UINT8*)(initrd_p + in->sec * FSZ_SECSIZE);
            } else {
                //sector directory
                ret.ptr=(UINT8*)(initrd_p + (unsigned int)(((FSZ_SectorList *)(initrd_p+in->sec*FSZ_SECSIZE))->fid) * FSZ_SECSIZE);
            }
            return ret;
        }
    }
    return ret;
}
#endif

/**
 * cpio archive
 */
file_t cpio_initrd(unsigned char *initrd_p, char *kernel)
{
    unsigned char *ptr=initrd_p;
    int k;
    file_t ret = { NULL, 0 };
    if(initrd_p==NULL || kernel==NULL || 
        (CompareMem(initrd_p,"070701",6) && CompareMem(initrd_p,"070702",6) && CompareMem(initrd_p,"070707",6)))
        return ret;
    DBG(L" * cpio %s\n",a2u(kernel));
    k=strlena((unsigned char*)kernel);
    // hpodc archive
    while(!CompareMem(ptr,"070707",6)){
        int ns=oct2bin(ptr+8*6+11,6);
        int fs=oct2bin(ptr+8*6+11+6,11);
        if(!CompareMem(ptr+9*6+2*11,kernel,k+1)){
            ret.size=fs;
            ret.ptr=(UINT8*)(ptr+9*6+2*11+ns);
            return ret;
        }
        ptr+=(76+ns+fs);
    }
    // newc and crc archive
    while(!CompareMem(ptr,"07070",5)){
        int fs=hex2bin(ptr+8*6+6,8);
        int ns=hex2bin(ptr+8*11+6,8);
        if(!CompareMem(ptr+110,kernel,k+1)){
            ret.size=fs;
            ret.ptr=(UINT8*)(ptr+((110+ns+3)/4)*4);
            return ret;
        }
        ptr+=((110+ns+3)/4)*4 + ((fs+3)/4)*4;
    }
    return ret;
}

/**
 * ustar tarball archive
 */
file_t tar_initrd(unsigned char *initrd_p, char *kernel)
{
    unsigned char *ptr=initrd_p;
    int k;
    file_t ret = { NULL, 0 };
    if(initrd_p==NULL || kernel==NULL || CompareMem(initrd_p+257,"ustar",5))
        return ret;
    DBG(L" * tar %s\n",a2u(kernel));
    k=strlena((unsigned char*)kernel);
    while(!CompareMem(ptr+257,"ustar",5)){
        int fs=oct2bin(ptr+0x7c,11);
        if(!CompareMem(ptr,kernel,k+1)){
            ret.size=fs;
            ret.ptr=(UINT8*)(ptr+512);
            return ret;
        }
        ptr+=(((fs+511)/512)+1)*512;
    }
    return ret;
}

/**
 * Static file system drivers registry
 */
file_t (*fsdrivers[]) (unsigned char *, char *) = {
#ifdef _FS_Z_H_
    fsz_initrd,
#endif
    cpio_initrd,
    tar_initrd,
    NULL
};
