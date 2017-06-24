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

void dev_link(FSZ_DirEnt *entries, char *name, ino_t inode)
{
    FSZ_DirEntHeader *hdr = (FSZ_DirEntHeader *)entries;
    hdr->numentries++;
    entries += hdr->numentries;
    entries->fid = inode;
    entries->length = mblen(name,FILENAME_MAX);
    memcpy(entries->name,name,strnlen(name,FILENAME_MAX));
    hdr->checksum=crc32_calc((char*)hdr+sizeof(FSZ_DirEntHeader),hdr->numentries*sizeof(FSZ_DirEnt));
}

void devfs_init()
{
    ino_t in;
    inode_t inode;
    /* add it here, because there's no better place */
    fsdrv_t tmpdrv = {
        "tmpfs",
        "Memory disk",
        NULL
    };
    _vfs_regfs(&tmpdrv);

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

    //VFS_INODE_ZERODEV
    memzero((void*)&inode,sizeof(inode_t));
    inode.nlink=1;
    inode.type=VFS_INODE_TYPE_BLKDEV;
    inode.blkdev.drivertask = VFS_BLKDEV_MEMFS;
    inode.blkdev.device = VFS_MEMFS_ZERO_DEVICE;
    inode.blkdev.blksize = __PAGESIZE;
    dev_link(devdir,"zero",vfs_inode(&inode));

    //VFS_INODE_RAMDISK
    memzero((void*)&inode,sizeof(inode_t));
    inode.nlink=1;
    inode.type=VFS_INODE_TYPE_BLKDEV;
    inode.blkdev.drivertask = VFS_BLKDEV_MEMFS;
    inode.blkdev.device = VFS_MEMFS_RAMDISK_DEVICE;
    inode.blkdev.startsec = (uint64_t)_initrd_ptr;
    inode.blkdev.size = (_initrd_size+__PAGESIZE-1)/__PAGESIZE;
    inode.blkdev.blksize = __PAGESIZE;
    dev_link(devdir,"root",vfs_inode(&inode));

    //VFS_INODE_RNDDEV
    memzero((void*)&inode,sizeof(inode_t));
    inode.nlink=1;
    inode.type=VFS_INODE_TYPE_BLKDEV;
    inode.blkdev.drivertask = VFS_BLKDEV_MEMFS;
    inode.blkdev.device = VFS_MEMFS_RANDOM_DEVICE;
    inode.blkdev.startsec = 0;
    inode.blkdev.size = 0;
    inode.blkdev.blksize = __PAGESIZE;
    dev_link(devdir,"random",vfs_inode(&inode));

    //This is basically a list /dev/
    memzero((void*)&inode,sizeof(inode_t));
    inode.nlink=1;
    inode.type=VFS_INODE_TYPE_DIRECTORY;
    inode.directory.entries = devdir;
    cache_dir("/dev",vfs_inode(&inode));

    // subdirectory /dev/vol/
    memzero((void*)&inode,sizeof(inode_t));
    inode.nlink=1;
    inode.type=VFS_INODE_TYPE_DIRECTORY;
    inode.directory.entries = devvoldir;
    in=vfs_inode(&inode);
    dev_link(devdir,"vol/",in);
    cache_dir("/dev/vol",in);

    // subdirectory /dev/uuid/
    memzero((void*)&inode,sizeof(inode_t));
    inode.nlink=1;
    inode.type=VFS_INODE_TYPE_DIRECTORY;
    inode.directory.entries = devuuiddir;
    in=vfs_inode(&inode);
    dev_link(devdir,"uuid/",in);
    cache_dir("/dev/uuid",in);
}
