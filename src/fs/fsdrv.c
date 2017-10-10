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

/* devfs index in fsdrv list */
uint16_t devfsidx = -1;

/* filesystem parsers */
uint16_t nfsdrv = 0;
fsdrv_t *fsdrv = NULL;

/**
 * Register a filesystem parser
 */
public uint16_t fsdrv_reg(const fsdrv_t *fs)
{
    bool_t isdevfs;
    // failsafe
    if(fs->name==NULL || fs->name[0]==0)
        return -1;
    // only devfs allowed to omit detect and locate functions,
    // and there it's mandatory to avoid false positives
    isdevfs=!memcmp(fs->name,"devfs",6);
    if(!isdevfs && (fs->detect==NULL || fs->locate==NULL))
        return -1;
    fsdrv=(fsdrv_t*)realloc(fsdrv,++nfsdrv*sizeof(fsdrv_t));
    // abort if we cannot allocate file system drivers
    if(fsdrv==NULL || errno())
        exit(2);
    memcpy((void*)&fsdrv[nfsdrv-1], (void*)fs, sizeof(fsdrv_t));
    // record devfs index
    if(devfsidx==(uint16_t)-1 && isdevfs)
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
    void *blk=readblock(dev, 0);
    if(blk!=NULL) {
        // iterate on each fsdrv, see if one can recognize the superblock
        // normally there should be no fsdrv without detect, but be sure
        for(i=0;i<nfsdrv;i++) {
            if(fsdrv[i].detect!=NULL && (*fsdrv[i].detect)(dev,blk))
                return i;
        }
    }
    return -1;
}

#if DEBUG
void fsdrv_dump()
{
    int i;
    dbg_printf("File system drivers %d:\n",nfsdrv);
    for(i=0;i<nfsdrv;i++) {
        dbg_printf("%3d. '%s' %s %x:",i,fsdrv[i].name,fsdrv[i].desc,fsdrv[i].detect);
        if(i==devfsidx) {
            dbg_printf(" (built-in)\n");
        } else {
            if(fsdrv[i].detect) dbg_printf(" detect");
            if(fsdrv[i].locate) dbg_printf(" locate");
            if(fsdrv[i].resizefs) dbg_printf(" resizefs");
            if(fsdrv[i].checkfs) dbg_printf(" checkfs");
            if(fsdrv[i].stat) dbg_printf(" stat");
            if(fsdrv[i].read) dbg_printf(" read");
            if(fsdrv[i].write) dbg_printf(" write");
            if(fsdrv[i].getdirent) dbg_printf(" getdirent");
            dbg_printf("\n");
        }
    }
}
#endif
