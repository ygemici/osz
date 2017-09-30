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
#include <vfs.h>
#include <fsZ.h>
#include <crc32.h>

#define MAXINDIRECT 8

/**
 * check magic to detect file system
 */
bool_t detect(void *blk)
{
    return
     !memcmp(((FSZ_SuperBlock *)blk)->magic, FSZ_MAGIC,4) &&
     !memcmp(((FSZ_SuperBlock *)blk)->magic2,FSZ_MAGIC,4) &&
     ((FSZ_SuperBlock *)blk)->checksum == crc32_calc((char*)((FSZ_SuperBlock *)blk)->magic, 508)
     ? true : false;
}

/**
 * locate a file on a specific device with this file system
 */
uint8_t locate(fid_t storage, locate_t *loc)
{
    // failsafe
    if(loc==NULL || loc->path==NULL || loc->path[0]==0)
        return NOTFOUND;
    // get superblock
    FSZ_SuperBlock *sb=(FSZ_SuperBlock *)readblock(storage, 0, __PAGESIZE);
    if(sb==NULL)
        return NOBLOCK;
    FSZ_Inode *in;
    FSZ_DirEnt *dirent;
    ino_t lsn,*sdblk[MAXINDIRECT];
    uint16_t sdptr[MAXINDIRECT];
    uint64_t bs,bp,sdmax,i;
    char *e;

    // start with root directory
    bs=1<<(sb->logsec+11);
    lsn=sb->rootdirfid;
    sdmax=bs/16;
nextdir:
dbg_printf("FS/Z locate %s %d.'%s'\n",fcb[storage].abspath,lsn,loc->path);
    for(e=loc->path;*e!='/' && *e!=0;e++);
    if(*e=='/') e++;
    // read inode
    in=(FSZ_Inode*)readblock(storage,lsn,bs);
    if(in==NULL)
        return NOBLOCK;
    if(memcmp(in->magic, FSZ_IN_MAGIC, 4))
        return FSERROR;
dbg_printf("  %04d: %x '%s' %x\n", lsn, in, in->filetype, FLAG_TRANSLATION(in->flags));

    // inode types with inlined data
    if(!memcmp(in->filetype, FILETYPE_SYMLINK, 4)) {
        loc->fileblk=in->inlinedata;
        return SYMINPATH;
    }

    if(!memcmp(in->filetype, FILETYPE_UNION, 4)) {
        loc->fileblk=in->inlinedata;
        return UNIONINPATH;
    }

    // get inode data
    memset(sdptr,0,MAXINDIRECT*sizeof(uint16_t));
    memset(sdblk,0,MAXINDIRECT*sizeof(void*));
    switch(FLAG_TRANSLATION(in->flags)) {
        case FSZ_IN_FLAG_INLINE:
            loc->fileblk=in->inlinedata;
            break;
        case FSZ_IN_FLAG_SECLIST:
        case FSZ_IN_FLAG_SECLIST0:
        case FSZ_IN_FLAG_SECLIST1:
        case FSZ_IN_FLAG_SECLIST2:
            // TODO: extents
            break;
        case FSZ_IN_FLAG_DIRECT:
            loc->fileblk=readblock(storage, in->sec, bs);
            if(loc->fileblk==NULL)
                return NOBLOCK;
            break;
        default:
            // get the first level sd
            sdblk[0]=readblock(storage, in->sec, bs);
            if(sdblk[0]==NULL)
                return NOBLOCK;
            // failsafe
            if(FLAG_TRANSLATION(in->flags)>=MAXINDIRECT)
                return FSERROR;
            // load each sd along the indirection path
            for(i=1;i<FLAG_TRANSLATION(in->flags);i++) {
                sdblk[i]=readblock(storage, sdblk[i-1][0], bs);
                if(sdblk[i]==NULL)
                    return NOBLOCK;
            }
            // load the first sector pointed by the last indirect sd
            loc->fileblk=readblock(storage, sdblk[i-1][0], bs);
            break;
    }

    if(!memcmp(in->filetype, FILETYPE_REG_APP, 4)) {
        return FILEINPATH;
    } else

    if(!memcmp(in->filetype, FILETYPE_DIR, 4)) {
        dirent=(FSZ_DirEnt *)(loc->fileblk);
        if(memcmp(((FSZ_DirEntHeader *)dirent)->magic, FSZ_DIR_MAGIC, 4))
            return FSERROR;
        i=((FSZ_DirEntHeader *)dirent)->numentries;
        bp=0;
        while(i) {
            dirent++; bp+=sizeof(FSZ_DirEnt);
            if(bp>=bs) {
                // FIXME: block end reached, load next block from sdblk and alter dirent pointer
            }
            if(dirent->name[0]==0) break;
            if(!memcmp(dirent->name,loc->path,e-loc->path)) {
                // end of path
                if(*e==0 && (dirent->name[e-loc->path]==0 || dirent->name[e-loc->path]=='/')) {
                    loc->result = dirent->fid;
                    return SUCCESS;
                }
                // path continues
                pathpush(lsn);
                lsn=dirent->fid;
                loc->path=e;
                goto nextdir;
            }
            i--;
        }
    } else
        return NOTFOUND;

    return SUCCESS;
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
