/*
 * fs/devfs.c
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
 * @brief Device File System. It's not a real fs
 */
#include <osZ.h>
#include "devfs.h"
#include "fsdrv.h"
#include "fcb.h"
#include "vfs.h"

/* ramdisk position and size in memory */
extern uint64_t _initrd_size;

/* devices */
uint64_t ndev = 0;
devfs_t *dev = NULL;

/**
 * add a device
 */
uint64_t devfs_add(char *name, pid_t drivertask, dev_t device, mode_t mode, blksize_t blksize, blkcnt_t blkcnt)
{
    uint64_t i;
    fid_t f;
    char tmp[64+sizeof(DEVPATH)]=DEVPATH;
    strncpy(&tmp[5], name, 64);
    for(i=0;i<ndev;i++)
        if(!strcmp(fcb[dev[i].fid].abspath, tmp))
            return i;
    ndev++;
    dev=(devfs_t*)realloc(dev, ndev*sizeof(devfs_t));
    if(dev==NULL)
        return -1;
    f=fcb_add(tmp, FCB_TYPE_DEVICE);
    fcb[f].nopen++;
    fcb[f].mode=mode;
    fcb[f].device.inode=i;
    fcb[i].device.blksize=blksize;
    fcb[f].device.filesize=blksize*blkcnt;
    fcb[f].device.storage=DEVFCB;
    dev[i].fid=f;
    dev[i].drivertask=drivertask;
    dev[i].device=device;
    return i;
}

/**
 * initialize devfs
 */
void devfs_init()
{
    fsdrv_t devdrv = {
        "devfs",
        "Device list",
        NULL,
        NULL
    };
    fsdrv_reg(&devdrv);

    fcb_add(DEVPATH, FCB_TYPE_REG_DIR);
    if(devfs_add("zero", MEMFS_MAJOR, MEMFS_ZERO, O_RDWR, __PAGESIZE, 1)==-1) abort();
    if(devfs_add("root", MEMFS_MAJOR, MEMFS_RAMDISK, O_RDWR, __PAGESIZE, (_initrd_size+__PAGESIZE-1)/__PAGESIZE)==-1) abort();
    if(devfs_add("random", MEMFS_MAJOR, MEMFS_RANDOM, O_RDONLY, __PAGESIZE, 1)==-1) abort();
    if(devfs_add("null", MEMFS_MAJOR, MEMFS_NULL, O_RDWR, __PAGESIZE, 0)==-1) abort();
    if(devfs_add("tmp", MEMFS_MAJOR, MEMFS_TMPFS, O_RDWR, __PAGESIZE, 0)==-1) abort();
}

#if DEBUG
void devfs_dump()
{
    int i;
    dbg_printf("Devices %d:\n",ndev);
    for(i=0;i<ndev;i++) {
        dbg_printf("%3d. device pid %x dev %x blk %d",
            i,dev[i].drivertask,dev[i].device,fcb[dev[i].fid].device.blksize);
        dbg_printf(" size %d %d %s\n",fcb[dev[i].fid].device.filesize,dev[i].fid,fcb[dev[i].fid].abspath);
    }
}
#endif
