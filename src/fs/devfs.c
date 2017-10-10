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
#include "mtab.h"
#include "devfs.h"
#include "fsdrv.h"
#include "fcb.h"
#include "vfs.h"

/* ramdisk position and size in memory */
extern uint64_t _initrd_size;

/* devices */
uint64_t ndev = 0;
devfs_t *dev = NULL;

uint64_t dev_recent = 0;

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
    // abort if we cannot allocate devfs
    if(dev==NULL)
        exit(2);
    f=fcb_add(tmp, FCB_TYPE_DEVICE);
    fcb[f].nopen++;
    fcb[f].mode=mode & O_AMSK;
    fcb[f].device.inode=i;
    fcb[f].device.blksize=blksize;
    fcb[f].device.filesize=blksize*blkcnt;
    fcb[f].device.storage=DEVFCB;
    dev[i].fid=f;
    dev[i].drivertask=drivertask;
    dev[i].device=device;
    dev[i].recent=++dev_recent;
    return i;
}

/**
 * remove a device
 */
void devfs_del(uint64_t idx)
{
    int i;
    if(idx==-1 || idx>=ndev)
        return;
    
    // remove from mtab
    for(i=0;i<nmtab;i++)
        if(mtab[i].storage==dev[idx].fid) {
            fcb_del(mtab[i].mountpoint);
            nmtab--;
            memcpy(&mtab[i], &mtab[i+1], (nmtab-i)*sizeof(mtab_t));
            mtab=(mtab_t*)realloc(mtab, nmtab*sizeof(mtab_t));
        }
    // remove all files referenced on this device
    for(i=0;i<nfcb;i++)
        if(fcb[i].type<FCB_TYPE_PIPE && fcb[i].reg.storage==dev[idx].fid) {
            fcb[i].nopen=0;
            fcb_del(i);
        }
    // free all blocks
    cache_freeblocks(dev[idx].fid, 0);
    // remove the device from file list
    fcb[dev[idx].fid].nopen=0;
    fcb_del(dev[idx].fid);
    // remove from devfs
    dev[idx].fid=-1;
    i=ndev; while(i>0 && dev[i-1].fid==-1) i--;
    if(i!=ndev) {
        ndev=i;
        dev=(devfs_t*)realloc(dev,ndev*sizeof(devfs_t));
    }
    // we don't remove taskctx with open files on the device, as they will
    // automatically report feof() / ferror() on next operation
}

/**
 * mark a device used
 */
void devfs_used(uint64_t idx)
{
    uint64_t i;
    if(idx==-1 || idx>=ndev)
        return;
    // overflow protection, just to be bullet proof :-)
    if(dev_recent>0xFFFFFFFFFFFFFFFUL) {
        dev_recent>>=48;
        for(i=0;i<ndev;i++)
            if(dev[i].fid!=-1) {
                dev[i].recent>>=48;
                dev[i].recent++;
            }
    }
    dev[idx].recent=++dev_recent;
}

/**
 * get the last recently used device
 */
uint64_t devfs_lastused(bool_t all)
{
    uint64_t i,l=0,k=dev_recent;
    for(i=0;i<ndev;i++)
        if(dev[i].fid!=-1 && dev[i].recent<k && (all || dev[i].recent!=0)) {
            k=dev[i].recent;
            l=i;
        }
    if(!all)
        dev[l].recent=0;
    return l;
}

/**
 * return a dirent_t for devfs entry
 */
public size_t devfs_getdirent(fpos_t offs)
{
    fcb_t *f;
    size_t s=1;
    // handle empty slots
    while(offs!=-1 && offs<ndev && dev[offs].fid==-1) { s++; offs++; }
    // handle EOF
    if(offs==-1 || offs>=ndev) return 0;
    // fill in dirent struct fields
    f=&fcb[dev[offs].fid];
    dirent.d_dev=DEVFCB;
    dirent.d_ino=offs;
    memcpy(&dirent.d_icon,"dev",3);
    dirent.d_filesize=f->device.filesize;
    dirent.d_type=FCB_TYPE_DEVICE | (f->device.blksize<=1?S_IFCHR>>16:0);
    strcpy(dirent.d_name,f->abspath + strlen(DEVPATH));
    dirent.d_len=strlen(dirent.d_name);
    return s;
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
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
    };
    fsdrv_reg(&devdrv);

    fcb_add(DEVPATH, FCB_TYPE_REG_DIR);
    if(devfs_add("zero", MEMFS_MAJOR, MEMFS_ZERO, O_RDONLY, __PAGESIZE, 1)==-1) exit(2);
    if(devfs_add("root", MEMFS_MAJOR, MEMFS_RAMDISK, O_RDWR, __PAGESIZE, (_initrd_size+__PAGESIZE-1)/__PAGESIZE)
        ==-1) exit(2);
    if(devfs_add("random", MEMFS_MAJOR, MEMFS_RANDOM, O_RDONLY, __PAGESIZE, 1)==-1) exit(2);
    if(devfs_add("null", MEMFS_MAJOR, MEMFS_NULL, O_WRONLY, __PAGESIZE, 0)==-1) exit(2);

}

#if DEBUG
void devfs_dump()
{
    int i;
    dbg_printf("Devices %d:\n",ndev);
    for(i=0;i<ndev;i++) {
        if(dev[i].fid==-1)
            continue;
        dbg_printf("%3d. pid %02x dev %x mode %2x blk %d",
            i,dev[i].drivertask,dev[i].device,fcb[dev[i].fid].mode,fcb[dev[i].fid].device.blksize);
        dbg_printf(" size %d %d %s\n",fcb[dev[i].fid].device.filesize,dev[i].fid,fcb[dev[i].fid].abspath);
    }
}
#endif
