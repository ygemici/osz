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

void resizefs(fid_t fd);

/**
 * check magic to detect file system
 */
bool_t detect(fid_t fd, void *blk)
{
    FSZ_SuperBlock *sb=(FSZ_SuperBlock *)blk;
    bool_t ret=!memcmp(sb->magic, FSZ_MAGIC,4) && !memcmp(sb->magic2,FSZ_MAGIC,4) &&
        sb->checksum == crc32_calc((char*)&sb->magic, 508) ? true : false;
    if(ret) {
        // calculate block size for this instance
        fcb[fd].device.blksize=1<<(sb->logsec+11);
        // check if file system size is correct
        if(fcb[fd].device.filesize != sb->numsec*fcb[fd].device.blksize)
            resizefs(fd);
    }
    return ret;
}

/**
 * locate a file on a specific device with this file system
 */
uint8_t locate(fid_t fd, ino_t parent, locate_t *loc)
{
    // failsafe
    if(loc==NULL || loc->path==NULL || fd>=nfcb || fcb[fd].abspath==NULL)
        return NOTFOUND;
    // get superblock
    FSZ_SuperBlock *sb=(FSZ_SuperBlock *)readblock(fd, 0);
    if(sb==NULL)
        return NOBLOCK;
    FSZ_Inode *in;
    FSZ_DirEnt *dirent;
    ino_t lsn,*sdblk[MAXINDIRECT];
    uint16_t sdptr[MAXINDIRECT];
    uint64_t bp,sdmax,i;
    char *e;

    // start with root directory if parent not given
    lsn=parent==0? sb->rootdirfid : parent;
    sdmax=fcb[fd].device.blksize/16;
nextdir:
    for(e=loc->path;*e!='/' && !PATHEND(*e);e++);
    if(*e=='/') e++;
    // handle up directory
    if(e-loc->path>=2 && loc->path[0]=='.' && loc->path[1]=='.' && loc->path[2]!='.') {
        pathstack_t *last=pathpop();
        if(last==NULL) {
            return UPDIR;
        }
        lsn=last->inode;
        loc->path=last->path;
        strcpy(last->path,e);
        goto nextdir;
    }
    // read inode
    if(lsn==0)
        return NOBLOCK;
    in=(FSZ_Inode*)readblock(fd,lsn);
    if(in==NULL)
        return NOBLOCK;
    if(memcmp(in->magic, FSZ_IN_MAGIC, 4))
        return FSERROR;
//dbg_printf("  %04d: %x '%s' %x\n", lsn, in, in->filetype, FLAG_TRANSLATION(in->flags));

    // inode types with inlined data
    if(!memcmp(in->filetype, FILETYPE_SYMLINK, 4)) {
        loc->fileblk=in->inlinedata;
        return SYMINPATH;
    }

    // end of path
    if(PATHEND(*loc->path)) {
        loc->inode = lsn;
        loc->filesize = in->size;
        if(!memcmp(in->filetype, FILETYPE_DIR, 4))
            loc->type = FCB_TYPE_REG_DIR;
        else
        if(!memcmp(in->filetype, FILETYPE_UNION, 4))
            loc->type = FCB_TYPE_UNION;
        else
            loc->type = FCB_TYPE_REG_FILE;
        return SUCCESS;
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
            if(in->sec==0)
                return NOBLOCK;
            loc->fileblk=readblock(fd, in->sec);
            if(loc->fileblk==NULL)
                return NOBLOCK;
            break;
        default:
            // get the first level sd
            if(in->sec==0)
                return NOBLOCK;
            sdblk[0]=readblock(fd, in->sec);
            if(sdblk[0]==NULL)
                return NOBLOCK;
            // failsafe
            if(FLAG_TRANSLATION(in->flags)>=MAXINDIRECT)
                return FSERROR;
            // load each sd along the indirection path
            for(i=1;i<FLAG_TRANSLATION(in->flags);i++) {
                if(sdblk[i-1][0]==0)
                    return NOBLOCK;
                sdblk[i]=readblock(fd, sdblk[i-1][0]);
                if(sdblk[i]==NULL)
                    return NOBLOCK;
            }
            // load the first sector pointed by the last indirect sd
            if(sdblk[i-1][0]==0)
                return NOBLOCK;
            loc->fileblk=readblock(fd, sdblk[i-1][0]);
            if(loc->fileblk==NULL)
                return NOBLOCK;
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
            if(bp>=fcb[fd].device.blksize) {
                // FIXME: block end reached, load next block from sdblk and alter dirent pointer
                // make gcc happy until then
                sdmax--; sdmax++;
            }
            if(dirent->name[0]==0) break;
            // check for '...' joker in path
            if(e-loc->path>=3 && loc->path[0]=='.' && loc->path[1]=='.' && loc->path[2]=='.') {
                // only dive into sub-directories
                if(dirent->name[strlen((char*)dirent->name)-1]=='/') {
                    locate_t l;
                    l.path=e;
                    if(locate(fd, dirent->fid, &l)==SUCCESS) {
                        char *c=strdup(e);
                        if(c==NULL) {
                            // keeps errno which is set to ENOMEM
                            return NOBLOCK;
                        }
                        strcpy(loc->path, (char*)dirent->name);
                        strcat(loc->path, c);
                        free(c);
                        loc->fileblk=l.fileblk;
                        loc->filesize=l.filesize;
                        loc->inode=l.inode;
                        loc->type=l.type;
                        return SUCCESS;
                    }
                }
            } else
            // simple filename match
            if(!memcmp(dirent->name,loc->path,e-loc->path)) {
                pathpush(lsn, loc->path);
                lsn=dirent->fid;
                loc->path=e;
                goto nextdir;
            }
            i--;
        }
    }
    // TODO: if loc->creat, then create dir or path
    return NOTFOUND;
}

/**
 * resize file system
 */
void resizefs(fid_t fd)
{
//    FSZ_SuperBlock *sb=(FSZ_SuperBlock *)readblock(fd,0);
//    dbg_printf("FS/Z resizefs, currently %d should be %d\n",
//      sb->numsec, fcb[fd].device.filesize/fcb[fd].device.blksize);
}

/**
 * query stat_t from inode
 */
bool_t stat(fid_t fd, ino_t file, stat_t *st)
{
    FSZ_Inode *in=(FSZ_Inode*)readblock(fd,file);
//dbg_printf("fsz stat storage %d file %d\n",fd,file);
    lastlink=NULL;
    // failsafe
    if( fd==-1 || fd>=nfcb || fcb[fd].device.blksize==0 ||
        in==NULL || memcmp(in->magic, FSZ_IN_MAGIC, 4))
            return false;
    memcpy(st->st_type,in->filetype,4);
    if(in->filetype[3]!=':')
        memcpy(st->st_mime,in->mimetype,44);
    if(!memcmp(in->filetype,FILETYPE_SYMLINK,4)) {
        lastlink=(char*)in->inlinedata;
        st->st_mode &= ~S_IFMT;
        st->st_mode |= S_IFREG;
        st->st_mode |= S_IFLNK;
    }
    if(!memcmp(in->filetype,FILETYPE_UNION,4)) {
        st->st_mode &= ~S_IFMT;
        st->st_mode |= S_IFDIR;
        st->st_mode |= S_IFUNI;
    }
    if(!memcmp(in->filetype,FILETYPE_DIR,4)) {
        st->st_mode &= ~S_IFMT;
        st->st_mode |= S_IFDIR;
    }
    st->st_nlink=in->numlinks;
    memcpy((void*)&st->st_owner,(void*)&in->owner,sizeof(uuid_t));
    st->st_size=in->size;
    st->st_blocks=in->numblocks;
    st->st_atime=in->accessdate;
    st->st_ctime=in->changedate;
    st->st_mtime=in->modifydate;
    return true;
}

/**
 * read a block from a file. returns:
 *   NULL && ackdelayed: block not found in cache, retry when driver has loaded the sector
 *   *s==0: eof
 *   NULL && *s!=0: spare record, zeros
 *   !NULL && *s!=0: valid file data
 */
void *read(fid_t fd, ino_t file, fpos_t offs, size_t *s)
{
    FSZ_Inode *in=(FSZ_Inode*)readblock(fd,file);
    void *blk;
    uint64_t i,j,k,l,bs;
//dbg_printf("fsz read storage %d file %d offs %d s %d\n",fd,file,offs,*s);
    // failsafe
    if( file==0 || fd==-1 || fd>=nfcb || fcb[fd].device.blksize==0 ||
        in==NULL || memcmp(in->magic, FSZ_IN_MAGIC, 4) || in->size<=offs) goto read_err;
    bs=fcb[fd].device.blksize;
    // eof check when reading from end of file
    if(offs+*s > in->size) {
        *s = in->size-offs;
        // failsafe
        if(*s==0) return NULL;
    }
    l=FLAG_TRANSLATION(in->flags);
    // simplest case, data inlined in inode
    if(l==FSZ_IN_FLAG_INLINE || !memcmp(in->filetype,FILETYPE_SYMLINK,4) || !memcmp(in->filetype,FILETYPE_UNION,4))
        return (void*)(in->inlinedata + offs);
    // one block referenced directly
    if(in->sec==0)
        return NULL;
    blk=readblock(fd,in->sec);
    if(blk==NULL) goto read_err;
    if(l==FSZ_IN_FLAG_DIRECT)
        return (void*)(blk + offs);
    // otherwise translate sector directories
    // first block?
    if(offs<bs) {
        if(*s>bs-offs)
            *s=bs-offs; // max size returned from the first block
        offs=0;
    } else {
        offs/=bs;       // use block aligned offset
        offs*=bs;
        if(offs+*s > in->size) {
            *s = in->size-offs;
            if(*s==0) return NULL;
        }
    }
    // get how much data one sd entry covers at top level
    j=1; for(i=0;i<l;i++) j*=bs;
//dbg_printf(" indirect %d j %d offs %d s %d\n",l,j,offs,*s);
    // iterate through sd levels
    for(k=0;k<l;k++) {
        // get which sd entry belongs to this offset at level k
        i=(offs/j);
//dbg_printf(" level %d: sd %d blk %x ",k,i,blk);
        i=((uint64_t*)blk)[i<<1];
//dbg_printf(" next %d\n",i);
        // spare portion of file?
        if(i==0) return NULL;
        blk=readblock(fd,i);
        if(blk==NULL) goto read_err;
        j/=bs;
    }
//dbg_printf(" end %d s %d\n", offs%bs, *s);
    // okay, we got the data block at last
    return (void*)(blk+offs%bs);
    
read_err:
    *s = 0;
    return NULL;
}

/**
 * parse a directory entry in buffer into dirent and return it's size.
 */
size_t getdirent(void *buf, fpos_t offs, dirent_t *dirent)
{
    size_t s=0;
    if(buf==NULL)
        return 0;
    // skip directory header
    if(offs==0) {
        if(memcmp(((FSZ_DirEntHeader*)buf)->magic,FSZ_DIR_MAGIC,4))
            return 0;
        else {
            buf+=sizeof(FSZ_DirEntHeader);
            s=sizeof(FSZ_DirEntHeader);
        }
    }
    FSZ_DirEnt *ent=(FSZ_DirEnt*)buf;
    if(ent->fid==0 || ent->name[0]==0)
        return 0;
    dirent->d_ino=ent->fid;
    dirent->d_len=ent->length;
    memcpy(&dirent->d_name, ent->name, strlen((char*)ent->name));
    s+=sizeof(FSZ_DirEnt);
    return s;
}

void _init()
{
    fsdrv_t drv = {
        "fsz",
        "FS/Z",
        detect,
        locate,
        resizefs,
        stat,
        read,
        NULL,
        getdirent
    };
    //uint16_t id = 
    fsdrv_reg(&drv);
}
