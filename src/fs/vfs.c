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
#include "devfs.h"
#include "cache.h"

extern uint64_t _pathmax;

/* filesystem parsers */
uint16_t nfsdrvs = 0;
fsdrv_t *fsdrvs = NULL;

/* mount points */
uint64_t nmounts = 0;
mount_t *mounts = NULL;

/* files and directories */
uint64_t nfcbs = 0;
fcb_t *fcbs = NULL;

/* open files, returned by lsof */
uint64_t nfiles = 0;
openfile_t *files = NULL;

/**
 * Register a filesystem parser
 */
public uint16_t _fs_reg(const fsdrv_t *fs)
{
    fsdrvs=(fsdrv_t*)realloc(fsdrvs,++nfsdrvs*sizeof(fsdrv_t));
    if(!fsdrvs || errno)
        abort();
    memcpy((void*)&fsdrvs[nfsdrvs-1], (void*)fs, sizeof(fsdrv_t));
    return nfsdrvs-1;
}

/**
 * return filesystem parser
 */
uint16_t _fs_get(char *name)
{
    int i;
    for(i=0;i<nfsdrvs;i++)
        if(!strcmp(fsdrvs[i].name, name))
            return i;
    return 0;
}

/**
 * add a file to fcb list
 */
fid_t fcb_add(const fcb_t *fcb)
{
/*
    uint64_t i;
    for(i=0;i<nfcbs;i++) {
        if(fcbs[i].path!=NULL && !strcmp(fcbs[i].path,fcb->path))
            return i;
    }
*/
    fcbs=(fcb_t*)realloc(fcbs,++nfcbs*sizeof(fcb_t));
    if(!fcbs || errno)
        abort();
    memcpy((void*)&fcbs[nfcbs-1], (void*)fcb, sizeof(fcb_t));
    return nfcbs-1;
}

/**
 * parse /etc/fstab and build file system mount points
 */
void vfs_fstab(char *ptr, size_t size)
{
#if DEBUG
    dbg_printf("fstab: %x %d\n%s\n",ptr,size,ptr);
#endif
}


#if DEBUG
void vfs_dump()
{
    int i;
    dbg_printf("Filesystems %d:\n",nfsdrvs);
    for(i=0;i<nfsdrvs;i++)
        dbg_printf("%3d. '%s' %s %x\n",i,fsdrvs[i].name,fsdrvs[i].desc,fsdrvs[i].detect);

    dbg_printf("\nMounts %d:\n",nmounts);
    for(i=0;i<nmounts;i++)
        dbg_printf("%3d. '%s' %s %x\n",i,devdir[mounts[i].fs_spec].name,fsdrvs[mounts[i].fs_type].name,mounts[i].fs_file);

    dbg_printf("\nFile Control Blocks %d:\n",nfcbs);
    for(i=0;i<nfcbs;i++) {
        dbg_printf("%3d. %x ",i,fcbs[i].type);
        switch(fcbs[i].type) {
            case VFS_FCB_TYPE_FILE:
                dbg_printf("file %s\n",fcbs[i].path);
                break;
            case VFS_FCB_TYPE_PIPE:
                dbg_printf("pipe %s\n",fcbs[i].path);
                break;
            case VFS_FCB_TYPE_DIRECTORY:
                dbg_printf("directory %s\n",fcbs[i].path);
                break;
            default:
                dbg_printf("unknown type? %s\n",fcbs[i].path);
        }
    }

    dbg_printf("\nOpen files %d:\n",nfiles);
    for(i=0;i<nfiles;i++)
        dbg_printf("%3d. pid %x fid %5d seek %d\n",i,files[i].pid,files[i].fid,files[i].pos);

    dbg_printf("\n");
}
#endif
