/*
 * core/fs.c
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
 * @brief Pre FS task file loader to load files from an FS/Z initrd (read-only)
 */

#include <arch.h>
#include <fsZ.h>

dataseg uint64_t fs_size;

/**
 * return starting offset of file in identity mapped initrd
 * also sets fs_size variable, as it returns pointer
 */
void *fs_locate(char *fn)
{
    int i;
    /* WARNING relies on identity mapping */
    fs_size = 0;
    if(bootboot.initrd_ptr==0 || fn==NULL){
        return NULL;
    }
    /* FS/Z */
    if(!kmemcmp(((FSZ_SuperBlock *)bootboot.initrd_ptr)->magic,FSZ_MAGIC,4)) {
        FSZ_SuperBlock *sb = (FSZ_SuperBlock *)bootboot.initrd_ptr;
        FSZ_DirEnt *ent;
        FSZ_Inode *in=(FSZ_Inode *)(bootboot.initrd_ptr+sb->rootdirfid*FSZ_SECSIZE);
        // Get the inode from directory hierarchy
        char *s,*e;
        s=e=fn;
        i=0;
again:
        while(*e!='/'&&*e!=0&&*e!='\n'){e++;}
        if(*e=='/'){e++;}
        if(!kmemcmp(in->magic,FSZ_IN_MAGIC,4)){
            //is it inlined?
            if(!kmemcmp(in->inlinedata,FSZ_DIR_MAGIC,4)){
                ent=(FSZ_DirEnt *)(in->inlinedata);
            } else if(!kmemcmp((void *)(bootboot.initrd_ptr+in->sec*FSZ_SECSIZE),FSZ_DIR_MAGIC,4)){
                // go, get the sector pointed
                ent=(FSZ_DirEnt *)(bootboot.initrd_ptr+in->sec*FSZ_SECSIZE);
            } else {
                return NULL;
            }
            //skip header
            FSZ_DirEntHeader *hdr=(FSZ_DirEntHeader *)ent; ent++;
            //iterate on directory entries
            int j=hdr->numentries;
            while(j-->0){
                if(!kmemcmp(ent->name,s,e-s)) {
                    if(*e==0||*e=='\n') {
                        i=ent->fid;
                        break;
                    } else {
                        s=e;
                        in=(FSZ_Inode *)(bootboot.initrd_ptr+ent->fid*FSZ_SECSIZE);
                        goto again;
                    }
                }
                ent++;
            }
        } else {
            i=0;
        }
        // if we have an inode, load it's contents
        if(i!=0) {
            // fid -> inode ptr -> data ptr
            FSZ_Inode *in=(FSZ_Inode *)(bootboot.initrd_ptr+i*FSZ_SECSIZE);
            if(!kmemcmp(in->magic,FSZ_IN_MAGIC,4)){
                fs_size = in->size;
                if(in->sec==i) {
                    // inline data
                    return (void*)&in->inlinedata;
                } else {
                    if(in->size <= FSZ_SECSIZE)
                        // direct data
                        return (void*)(bootboot.initrd_ptr + in->sec * FSZ_SECSIZE);
                    else
                        // sector directory (only one level supported here, and no holes in files)
                        return (void*)(bootboot.initrd_ptr + (unsigned int)(((FSZ_SectorList *)(bootboot.initrd_ptr+in->sec*FSZ_SECSIZE))->fid) * FSZ_SECSIZE);
                }
            }
        }
    }
    return NULL;
}
