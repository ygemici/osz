/*
 * fs/fsdrv.c
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
 * @brief File System drivers
 */
#include <osZ.h>
#include "fsdrv.h"
#include "vfs.h"

uint16_t devfsidx = -1;

/* filesystem parsers */
uint16_t nfsdrv = 0;
fsdrv_t *fsdrv = NULL;

/**
 * Register a filesystem parser
 */
public uint16_t fsdrv_reg(const fsdrv_t *fs)
{
    fsdrv=(fsdrv_t*)realloc(fsdrv,++nfsdrv*sizeof(fsdrv_t));
    if(fsdrv==NULL || errno())
        abort();
    memcpy((void*)&fsdrv[nfsdrv-1], (void*)fs, sizeof(fsdrv_t));
    if(!memcmp(fs->name,"devfs",6))
        devfsidx = nfsdrv-1;
    return nfsdrv-1;
}

/**
 * return filesystem parser
 */
int16_t fsdrv_get(char *name)
{
    int i;
    for(i=0;i<nfsdrv;i++)
        if(!strcmp(fsdrv[i].name, name))
            return i;
    return -1;
}

/**
 * detect file system on a device (or in file)
 */
int16_t fsdrv_detect(fid_t dev)
{
    int i;
    // little hack as devfs does not have a superblock
    if(dev==DEVFCB)
        return devfsidx;
    // read the superblock
    void *blk=readblock(dev, 0, __PAGESIZE);
    if(blk!=NULL) {
        // iterate on each fsdrv, see if one can recognize the superblock
        for(i=0;i<nfsdrv;i++) {
            if(fsdrv[i].detect!=NULL && (*fsdrv[i].detect)(blk))
                return i;
        }
    }
    return -1;
}

#if DEBUG
void fsdrv_dump()
{
    int i;
    dbg_printf("Filesystems %d:\n",nfsdrv);
    for(i=0;i<nfsdrv;i++)
        dbg_printf("%3d. '%s' %s %x\n",i,fsdrv[i].name,fsdrv[i].desc,fsdrv[i].detect);
}
#endif
