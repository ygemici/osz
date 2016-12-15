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
 * @brief Pre FS service elf loader to load system services
 */

#include <elf.h>
#include <fsZ.h>

uint64_t __attribute__ ((section (".data"))) fs_size;

/* map any file from initrd into bss segment */
void *fs_mapfile(char *fn)
{
    return NULL;
}

/* map an ELF64 file from initrd into text segment */
void *fs_mapelf(char *fn)
{
    return NULL;
}

/* return starting offset of file in identity mapped initrd */
void *fs_locate(char *fn)
{
    fs_size = 0;
    /* WARNING relies on identity mapping */
    FSZ_SuperBlock *sb = (FSZ_SuperBlock *)bootboot.initrd_ptr;
    FSZ_DirEnt *ent;
    FSZ_Inode *in=(FSZ_Inode *)(bootboot.initrd_ptr+sb->rootdirfid*FSZ_SECSIZE);
    if(bootboot.initrd_ptr==0 || fn==NULL || kmemcmp(sb->magic,FSZ_MAGIC,4)){
        return NULL;
    }
    // Get the inode from directory hierarchy
    int i;
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
                    // sector directory (only one level supported here)
                    return (void*)(bootboot.initrd_ptr + (unsigned int)(((FSZ_SectorList *)(bootboot.initrd_ptr+in->sec*FSZ_SECSIZE))->fid) * FSZ_SECSIZE);
            }
        }
    }
    return NULL;
}

void fs_init()
{
    // this is so early, we don't have initrd in fs process' bss yet.
    // so we have to rely on identity mapping to locate the files
    char *s, *f, *drvs = (char *)fs_locate("etc/sys/drivers");
    char *drvs_end = drvs + fs_size;
    char fn[256];
    OSZ_tcb *tcb = (OSZ_tcb*)&tmp2map;
    pid_t pid = thread_new();
    fullsize = 0;
    // map device driver dispatcher
    thread_loadelf("sbin/fs");
    // map libc
    thread_loadso("lib/libc.so");
    // load filesystem drivers
    if(drvs==NULL) {
        // hardcoded devices if driver list not found
        thread_loadso("lib/sys/fs/gpt.so");
        thread_loadso("lib/sys/fs/fsz.so");
        thread_loadso("lib/sys/fs/vfat.so");
    } else {
        for(s=drvs;s<drvs_end;) {
            f = s; while(s<drvs_end && *s!=0 && *s!='\n') s++;
            if(f[0]=='*' && f[1]==9 && f[2]=='f' && f[3]=='s') {
                f+=2;
                if(s-f<255-8) {
                    kmemcpy(&fn[0], "lib/sys/", 8);
                    kmemcpy(&fn[8], f, s-f);
                    fn[s-f+8]=0;
                    thread_loadso(fn);
                }
                continue;
            }
            // failsafe
            if(s>=drvs_end || *s==0) break;
            if(*s=='\n') s++;
        }
    }

    // dynamic linker
    thread_dynlink(pid);
    // modify TCB for system task
    kmap((uint64_t)&tmp2map, (uint64_t)(pid*__PAGESIZE), PG_CORE_NOCACHE);
    tcb->linkmem += fullsize;
    thread_mapbss(bootboot.initrd_ptr, bootboot.initrd_size);

    // add to queue so that scheduler will know about this thread
    thread_add(pid);
}
