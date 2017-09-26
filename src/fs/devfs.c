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
#include <sys/driver.h>
#include <crc32.h>
#include "vfs.h"
#include "cache.h"
#include "devfs.h"

/* ramdisk position and size in memory */
extern uint8_t *_initrd_ptr;
extern uint64_t _initrd_size;

/* caller task of public service, see libc/dispatch.c */
extern pid_t mq_caller;

/* devices */
uint64_t ndevdir = 0;
device_t *devdir = NULL;
/* subdirectories */
FSZ_DirEnt *devvoldir;
FSZ_DirEnt *devuuiddir;

public uint64_t mknod()
{
    /* check mq_caller */
    return ndevdir;
}

/**
 * Link a file in a /dev subdirectory
 */
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

/**
 * locate a file in devfs. We don't use mnt here
 */
ino_t devfs_locate(mount_t *mnt, ino_t parent, char *file)
{
    uint64_t i;
    if(file==NULL || file[0]==0 || parent!=0)
        return -1;
    if(!strncmp(file,"vol/",4)) {
        file+=4;
        for(i=1;i<=((FSZ_DirEntHeader *)devvoldir)->numentries;i++)
            if(!strcmp(file,(char*)&devvoldir[i].name))
                return devvoldir[i].fid;
    } else
    if(!strncmp(file,"uuid/",5)) {
        file+=5;
        for(i=1;i<=((FSZ_DirEntHeader *)devuuiddir)->numentries;i++)
            if(!strcmp(file,(char*)&devuuiddir[i].name))
                return devuuiddir[i].fid;
    } else {
        for(i=0;i<ndevdir;i++) {
            if(!strcmp(file,devdir[i].name))
                return i;
        }
    }
    return -1;
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
        devfs_locate
    };
    fsdrv_reg(&devdrv);

    ndevdir=5;
    //allocate memory for devfs
    devdir=(device_t*)malloc(ndevdir*sizeof(device_t));
    if(!devdir || errno())
        abort();

    //allocate memory for dev subdirectories
    memcpy((void*)devdir,FSZ_DIR_MAGIC,4);
    devvoldir=(FSZ_DirEnt*)malloc(__PAGESIZE);
    if(!devvoldir || errno())
        abort();
    memcpy((void*)devvoldir,FSZ_DIR_MAGIC,4);
    devuuiddir=(FSZ_DirEnt*)malloc(__PAGESIZE);
    if(!devuuiddir || errno())
        abort();
    memcpy((void*)devuuiddir,FSZ_DIR_MAGIC,4);

    strcpy(devdir[0].name,"zero");
    devdir[0].drivertask = VFS_DEVICE_MEMFS;
    devdir[0].device = VFS_MEMFS_ZERO_DEVICE;
    devdir[0].blksize = __PAGESIZE;

    strcpy(devdir[1].name,"mem");
    devdir[1].drivertask = VFS_DEVICE_MEMFS;
    devdir[1].device = VFS_MEMFS_RAMDISK_DEVICE;
    devdir[1].startsec = (uint64_t)_initrd_ptr;
    devdir[1].size = (_initrd_size+__PAGESIZE-1)/__PAGESIZE;
    devdir[1].blksize = __PAGESIZE;

    strcpy(devdir[2].name,"random");
    devdir[2].drivertask = VFS_DEVICE_MEMFS;
    devdir[2].device = VFS_MEMFS_RANDOM_DEVICE;
    devdir[2].blksize = __PAGESIZE;

    strcpy(devdir[3].name,"null");
    devdir[3].drivertask = VFS_DEVICE_MEMFS;
    devdir[3].device = VFS_MEMFS_NULL_DEVICE;
    devdir[3].blksize = __PAGESIZE;

    strcpy(devdir[4].name,"tmp");
    devdir[4].drivertask = VFS_DEVICE_MEMFS;
    devdir[4].device = VFS_MEMFS_TMPFS_DEVICE;
    devdir[4].blksize = __PAGESIZE;
}

#if DEBUG
void devfs_dump()
{
    int i;
    dbg_printf("Devices %d:\n",ndevdir);
    for(i=0;i<ndevdir;i++) {
        dbg_printf("%3d. device pid %x dev %x start %x sec %d",
            i,devdir[i].drivertask,devdir[i].device,devdir[i].startsec,
            devdir[i].blksize);
        dbg_printf(" size %d %s\n",devdir[i].size,devdir[i].name);
    }
}
#endif
