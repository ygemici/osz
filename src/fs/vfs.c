/*
 * fs/vfs.c
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
 * @brief Virtual File System implementation
 */
#include <osZ.h>
#include "vfs.h"
#include "cache.h"

extern uint64_t _pathmax;
char *pathtmp;

/* initialize in memory filesystems */
extern void memfs_init();

/* filesystem parsers */
uint16_t nfsdrvs = 0;
fsdrv_t *fsdrvs = NULL;

/* files and directories */
uint64_t ninodes = 0;
inode_t *inodes = NULL;

/* open files, returned by lsof */
uint64_t nfiles = 0;
openfile_t *files = NULL;

public void mknod()
{
}

public ino_t getinode(ino_t parent,const char *path)
{
    if(parent>ninodes)
        parent=0;
    if(path==NULL||path[0]==0)
        return parent;
    dbg_printf("parent: '%s'\n", cachedir[inodes[parent].cachedir].name);
    return 0;
}

public uint16_t _vfs_regfs(const fsdrv_t *fs)
{
    fsdrvs=(fsdrv_t*)realloc(fsdrvs,++nfsdrvs*sizeof(fsdrv_t));
    if(!fsdrvs || errno)
        abort();
    memcpy((void*)&fsdrvs[nfsdrvs-1], (void*)fs, sizeof(fsdrv_t));
    return nfsdrvs-1;
}

ino_t vfs_inode()
{
    inodes=(inode_t*)realloc(inodes,++ninodes*sizeof(inode_t));
    if(!inodes || errno)
        abort();
    return ninodes-1;
}

void vfs_init()
{
    pathtmp=(char*)malloc(_pathmax<512?512:_pathmax);
    if(!pathtmp || errno)
        abort();

    /* initialize memory "block devices" */
    memfs_init();
    
    /* add root directory */
    cache_dir("/",vfs_inode());

    getinode(0,"/etc");
}

#if DEBUG
void vfs_dump()
{
    int i;
    dbg_printf("Filesystems %d:\n",nfsdrvs);
    for(i=0;i<nfsdrvs;i++)
        dbg_printf("%3d. '%s' %s %x\n",i,fsdrvs[i].name,fsdrvs[i].desc,fsdrvs[i].detect);

    dbg_printf("\nInodes %d:\n",ninodes);
    for(i=0;i<ninodes;i++)
        dbg_printf("%3d. %x %s type %x\n",i,inodes[i].fs,fsdrvs[inodes[i].fs].name,inodes[i].type);
    
    dbg_printf("\nOpen files %d:\n",nfiles);
    for(i=0;i<nfiles;i++)
        dbg_printf("%3d. pid %x inode %5d seek %d\n",i,files[i].pid,files[i].inode,files[i].pos);

    dbg_printf("\n");
}
#endif
