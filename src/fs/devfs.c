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
 * @brief Device File System. It's not a real fs, just a fake directory
 */
#include <osZ.h>
#include <crc32.h>
#include "vfs.h"
#include "cache.h"

extern uint8_t *_initrd_ptr;
extern uint64_t _initrd_size;

FSZ_DirEnt *devdir;
FSZ_DirEnt *devvoldir;
FSZ_DirEnt *devuuiddir;

public void mknod()
{
}

void dev_link(FSZ_DirEnt *entries, char *name, fid_t fid)
{
    FSZ_DirEntHeader *hdr = (FSZ_DirEntHeader *)entries;
    hdr->numentries++;
    entries += hdr->numentries;
    entries->fid = fid;
    entries->length = mblen(name,FILENAME_MAX);
    memcpy(entries->name,name,strnlen(name,FILENAME_MAX));
    hdr->checksum=crc32_calc((char*)hdr+sizeof(FSZ_DirEntHeader),hdr->numentries*sizeof(FSZ_DirEnt));
}

void devfs_init()
{
    fcb_t fcb;

    fsdrv_t devdrv = {
        "devfs",
        "Device list",
        NULL
    };
    _fs_reg(&devdrv);
    /* add it here, because there's no better place */
    fsdrv_t tmpdrv = {
        "tmpfs",
        "Memory disk",
        NULL
    };
    _fs_reg(&tmpdrv);

    //allocate memory for /dev/
    devdir=(FSZ_DirEnt*)malloc(__PAGESIZE);
    if(!devdir || errno)
        abort();
    memcpy((void*)devdir,FSZ_DIR_MAGIC,4);
    devvoldir=(FSZ_DirEnt*)malloc(__PAGESIZE);
    if(!devvoldir || errno)
        abort();
    memcpy((void*)devvoldir,FSZ_DIR_MAGIC,4);
    devuuiddir=(FSZ_DirEnt*)malloc(__PAGESIZE);
    if(!devuuiddir || errno)
        abort();
    memcpy((void*)devuuiddir,FSZ_DIR_MAGIC,4);

    //VFS_FCB_ZERODEV
    memzero((void*)&fcb,sizeof(fcb_t));
    fcb.nlink=1;
    fcb.path=malloc(10);
    strcpy(fcb.path,"/dev/zero");
    fcb.type=VFS_FCB_TYPE_DEVICE;
    fcb.dev.drivertask = VFS_DEVICE_MEMFS;
    fcb.dev.device = VFS_MEMFS_ZERO_DEVICE;
    fcb.dev.blksize = __PAGESIZE;
    dev_link(devdir,fcb.path+5,fcb_add(&fcb));

    //VFS_FCB_RAMDISK
    memzero((void*)&fcb,sizeof(fcb_t));
    fcb.nlink=1;
    fcb.path=malloc(9);
    strcpy(fcb.path,"/dev/mem");
    fcb.type=VFS_FCB_TYPE_DEVICE;
    fcb.dev.drivertask = VFS_DEVICE_MEMFS;
    fcb.dev.device = VFS_MEMFS_RAMDISK_DEVICE;
    fcb.dev.startsec = (uint64_t)_initrd_ptr;
    fcb.dev.size = (_initrd_size+__PAGESIZE-1)/__PAGESIZE;
    fcb.dev.blksize = __PAGESIZE;
    dev_link(devdir,fcb.path+5,fcb_add(&fcb));

    //VFS_FCB_RNDDEV
    memzero((void*)&fcb,sizeof(fcb_t));
    fcb.nlink=1;
    fcb.path=malloc(12);
    strcpy(fcb.path,"/dev/random");
    fcb.type=VFS_FCB_TYPE_DEVICE;
    fcb.dev.drivertask = VFS_DEVICE_MEMFS;
    fcb.dev.device = VFS_MEMFS_RANDOM_DEVICE;
    fcb.dev.blksize = __PAGESIZE;
    dev_link(devdir,fcb.path+5,fcb_add(&fcb));

    //VFS_FCB_NULLDEV
    memzero((void*)&fcb,sizeof(fcb_t));
    fcb.nlink=1;
    fcb.path=malloc(10);
    strcpy(fcb.path,"/dev/null");
    fcb.type=VFS_FCB_TYPE_DEVICE;
    fcb.dev.drivertask = VFS_DEVICE_MEMFS;
    fcb.dev.device = VFS_MEMFS_NULL_DEVICE;
    fcb.dev.blksize = __PAGESIZE;
    dev_link(devdir,fcb.path+5,fcb_add(&fcb));

    //VFS_FCB_TMPDEV
    memzero((void*)&fcb,sizeof(fcb_t));
    fcb.nlink=1;
    fcb.path=malloc(9);
    strcpy(fcb.path,"/dev/tmp");
    fcb.type=VFS_FCB_TYPE_DEVICE;
    fcb.dev.drivertask = VFS_DEVICE_MEMFS;
    fcb.dev.device = VFS_MEMFS_TMPFS_DEVICE;
    fcb.dev.blksize = __PAGESIZE;
    dev_link(devdir,fcb.path+5,fcb_add(&fcb));

}
