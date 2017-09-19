/*
 * drivers/fs/fsz/main.c
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
 * @brief FS/Z filesystem driver
 */

#include <osZ.h>
#include <cache.h>
#include <fsZ.h>
#include <crc32.h>

ino_t detect(void *blk)
{
    return
     !memcmp(((FSZ_SuperBlock *)blk)->magic, FSZ_MAGIC,4) &&
     !memcmp(((FSZ_SuperBlock *)blk)->magic2,FSZ_MAGIC,4) &&
     ((FSZ_SuperBlock *)blk)->checksum == crc32_calc((char*)((FSZ_SuperBlock *)blk)->magic, 508)
     ? ((FSZ_SuperBlock *)blk)->rootdirfid : -1;
}

ino_t locate(mount_t *mnt, ino_t parent, char *path)
{
    /* failsafe */
    if(mnt==NULL || path==NULL || path[0]==0)
        return -1;
    if(!mnt->rootdir) {
        FSZ_SuperBlock *sb=(FSZ_SuperBlock *)cache_getblock(mnt->fs_parent, mnt->fs_spec, 0);
        if(sb==NULL)
            return -1;
        mnt->rootdir=sb->rootdirfid;
    }
    if(parent==0)
        parent=mnt->rootdir;
again:
dbg_printf("FS/Z locate %d '%s'\n",parent,path);
    FSZ_Inode *in=(FSZ_Inode *)cache_getblock(mnt->fs_parent, mnt->fs_spec, parent);
    FSZ_DirEnt *dirent;
    char *e=path;
    uint64_t i=0;
    if(in==NULL)
        return -1;
    while(*e!='/' && *e!=0) e++;
    if(*e=='/') e++;
dbg_printf("%x '%s' %x\n", in, in->filetype, FLAG_TRANSLATION(in->flags));

    if(!memcmp(in->filetype, FILETYPE_SYMLINK, 4)) {
        return vfs_parsesymlink(path,in->inlinedata,in->size);
    }

    if(!memcmp(in->filetype, FILETYPE_UNION, 4)) {
        return vfs_parseunion(path,in->inlinedata,in->size);
    }

    if(!memcmp(in->filetype, FILETYPE_DIR, 4)) {
        dirent=NULL;
        if(FLAG_TRANSLATION(in->flags)==FSZ_IN_FLAG_INLINE) {
            // inline
            dirent=(FSZ_DirEnt *)(in->inlinedata);
        } else if(FLAG_TRANSLATION(in->flags)==FSZ_IN_FLAG_DIRECT) {
            // direct data
            dirent=(FSZ_DirEnt *)cache_getblock(mnt->fs_parent, mnt->fs_spec, in->sec);
        } else if(FLAG_TRANSLATION(in->flags)==FSZ_IN_FLAG_SD) {
            /* TODO: sector directory */
        }
        if(dirent==NULL)
            return -1;
        i=((FSZ_DirEntHeader *)dirent)->numentries;
        while(i) {
            dirent++;
            if(!memcmp(dirent->name,path,e-path)) {
                // end of path
                if(*e==0 && (dirent->name[e-path]==0 || dirent->name[e-path]=='/'))
                    return dirent->fid;
                // path continues
                parent=dirent->fid;
                path=e;
                goto again;
            }
            if(dirent->name[0]==0) break;
            i--;
        }
    }
    return -1;
}

void _init()
{
    fsdrv_t drv = {
        "fsz",
        "FS/Z",
        detect,
        locate
    };
    //uint16_t id = 
    fsdrv_reg(&drv);
}
