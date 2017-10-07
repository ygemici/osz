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
 * @brief Glue code between FS task and File System drivers
 */

#include <osZ.h>
#include <sys/driver.h>
#include "fcb.h"
#include "vfs.h"
#include "cache.h"
#include "devfs.h"
#include "mtab.h"

extern uint32_t pathmax;        // max length of path
extern uint64_t _initrd_ptr;    // /dev/root pointer and size
extern uint64_t _initrd_size;

void *zeroblk = NULL;           // zero block
void *rndblk = NULL;            // random data block

stat_t st;                      // buffer for lstat() and fstat()
dirent_t dirent;                // buffer for readdir()
public fsck_t fsck;             // fsck state block for dofsck() and fsdrv's checkfs()

int pathstackidx = 0;           // path stack
pathstack_t pathstack[PATHSTACKSIZE];

public char *lastlink;          // last symlink's target, filled in by fsdrv's stat()


/**
 * add an inode reference to path stack. Rotate if stack grows too big
 */
public void pathpush(ino_t lsn, char *path)
{
    if(pathstackidx==PATHSTACKSIZE) {
        memcpy(&pathstack[0], &pathstack[1], (PATHSTACKSIZE-1)*sizeof(pathstack_t));
        pathstack[PATHSTACKSIZE-1].inode = lsn;
        pathstack[PATHSTACKSIZE-1].path = path;
        return;
    }
    pathstack[pathstackidx].inode = lsn;
    pathstack[pathstackidx++].path = path;
}

/**
 * pop the last inode from path stack
 */
public pathstack_t *pathpop()
{
    if(pathstackidx==0)
        return NULL;
    pathstackidx--;
    return &pathstack[pathstackidx];
}

/**
 * append a filename to a directory path
 * path must be a sufficiently big buffer
 */
char *pathcat(char *path, char *filename)
{
    int i;
    if(path==NULL || path[0]==0)
        return NULL;
    if(filename==NULL || filename[0]==0)
        return path;
    i=strlen(path);
    if(i+strlen(filename)>=pathmax)
        return NULL;
    if(path[i-1]!='/') {
        path[i++]='/'; path[i]=0;
    }
    strcpy(path+i, filename + (filename[0]=='/'?1:0));
    return path;
}

/**
 * similar to realpath() but only uses memory, does not resolve
 * symlinks and directory up entries
 */
char *canonize(const char *path)
{
    int i=0,j=0,k,m;
    char *result;
    if(path==NULL || path[0]==0)
        return NULL;
    result=(char*)malloc(pathmax);
    if(result==NULL)
        return NULL;
    k=strlen(path);
    // translate dev: paths
    while(i<k && path[i]!=':' && path[i]!='/' && !PATHEND(path[i])) i++;
    if(path[i]==':') {
        // skip "root:"
        if(i==4 && !memcmp(path,"root",4)) {
            i=4; j=0;
        } else {
            strcpy(result, DEVPATH); j=sizeof(DEVPATH);
            strncpy(result+j, path, i); j+=i;
        }
        result[j++]='/'; m=j;
        i++;
        if(i<k && path[i]=='/') i++;
    } else {
        i=0;
        m=strlen(fcb[ctx->rootdir].abspath);
        if(path[0]=='/') {
            // absolute path
            strcpy(result, fcb[ctx->rootdir].abspath);
            j=m=strlen(result);
            if(result[j-1]=='/') i=1;
        } else {
            // use working directory
            strcpy(result, fcb[ctx->workdir].abspath);
            j=strlen(result);
        }
    }

    // parse the remaining part of the path
    while(i<k && !PATHEND(path[i])) {
        if(result[j-1]!='/') result[j++]='/';
        while(i<k && path[i]=='/') i++;
        // skip current dir paths
        if(path[i]=='.' && i+1<k && (path[i+1]=='/' || PATHEND(path[i+1]))) {
            i+=2; continue;
        } else
        // handle .../../ case: skip ../ and remove .../ from result
        if(j>4 && path[i]=='.' && i+2<k && path[i+1]=='.' && (path[i+2]=='/' || PATHEND(path[i+2])) &&
            !memcmp(result+j-4,".../",4)) {
                j-=4; i+=3; continue;
        // do not handle otherwise directory up here, as last directory
        // in result path could be a symlink
/*
        if(path[i]=='.') {
            i++;
            if(path[i]=='.') {
                i++;
                for(j--;j>m && result[j-1]!='/';j--);
                result[j]=0;
            }
*/
        } else {
            // copy directory name
            while(i<k && path[i]!='/' && !PATHEND(path[i]))
                result[j++]=path[i++];
            // canonize version (we do not use versioning for directories, as it would be
            // extremely costy. So only the last part, the filename may have version)
            if(path[i]==';') {
                // not for directories
                if(result[j-1]=='/')
                    break;
                // skip sign
                i++; if(i<k && path[i]=='-') i++;
                // only append if it's a valid number
                if(i+1<k && path[i]>='1' && path[i]<='9' && (path[i+1]=='#' || path[i+1]==0)) {
                    result[j++]=';';
                    result[j++]=path[i];
                    i++;
                }
                // no break, because offset may follow
            }
            // canonize offset (again, only the last filename may have offset)
            if(path[i]=='#') {
                // not for directories
                if(result[j-1]=='/')
                    break;
                // skip sign
                i++; if(i<k && path[i]=='-') i++;
                // only append if it's a valid number
                if(i<k && path[i]>='1' && path[i]<='9') {
                    result[j++]='#';
                    if(path[i-1]=='-')
                        result[j++]='-';
                    while(i<k && path[i]!=0 && path[i]>='0' && path[i]<='9')
                        result[j++]=path[i++];
                }
                // end of path for sure
                break;
            }
        }
        i++;
    }
    if(j>0 && result[j-1]=='/') j--;
    while(j>0 && result[j-1]=='.') j--;
    // trailing slash
    if(i>0 && path[i-1]=='/')
        result[j++]='/';
    result[j]=0;
    return result;
}

/**
 * get the version info from path
 */
public uint8_t getver(char *abspath)
{
    int i=0;
    if(abspath==NULL || abspath[0]==0)
        return 0;
    while(abspath[i]!=';' && abspath[i]!=0) i++;
    return abspath[i]==';' && abspath[i+1]>'0' && abspath[i+1]<='9' ? abspath[i+1]-'0' : 0;
}

/**
 * get the offset info from path
 */
public fpos_t getoffs(char *abspath)
{
    int i=0;
    if(abspath==NULL || abspath[0]==0)
        return 0;
    while(abspath[i]!='#' && abspath[i]!=0) i++;
    if(abspath[i]=='#')
        return atoll(&abspath[i+1]);
    return 0;
}

/**
 * read a block from an fcb entry (allowed when device is blocked)
 */
public void *readblock(fid_t fd, blkcnt_t lsn)
{
    devfs_t *device;
    void *blk=NULL, *ptr;
    writebuf_t *w;
    fcb_t *f;
    size_t bs;
    off_t o,e,we;
    // failsafe
    if(fd==-1 || fd>=nfcb || lsn==-1)
        return NULL;
#if DEBUG
    if(_debug&DBG_BLKIO)
        dbg_printf("FS: readblock(fd %d, sector %d)\n",fd,lsn);
#endif
    f=&fcb[fd];
    if(!(f->mode & O_READ)) { seterr(EACCES); return NULL; }
    switch(f->type) {
        case FCB_TYPE_REG_FILE:
            // reading a block from a regular file
            if(f->reg.storage==-1 || fcb[f->reg.storage].type!=FCB_TYPE_DEVICE ||
                f->fs >= nfsdrv || fsdrv[f->fs].read==NULL) { seterr(ENODEV); return NULL; }
            devfs_used(fcb[f->reg.storage].device.inode);
            bs=fcb[f->reg.storage].device.blksize;
            // check if block is in the write buf entirely
            o=lsn*bs; e=o+bs;
            for(w=f->reg.buf;w!=NULL;w=w->next) {
                //     ####
                //  +--------+
                if(o>=w->offs && e<=w->offs+w->size)
                    return (w->data + o-w->offs);
            }
            // valid address?
            if((lsn+1)*bs>f->reg.filesize) {
                seterr(EFAULT);
                return NULL;
            }
            // call file system driver to read block. This must return a cache block
            blk=(*fsdrv[f->fs].read)(f->reg.storage, f->reg.inode, lsn*bs, &bs);
            // skip if block is not in cache or eof
            if(ackdelayed || bs==0) return NULL;
            if(blk==NULL) {
                // spare portion of file? return zero block
                zeroblk=(void*)realloc(zeroblk, fcb[f->reg.storage].device.blksize);
                blk=zeroblk;
            }
            // Merging with write buf
            for(w=f->reg.buf;w!=NULL;w=w->next) {
                we=w->offs+w->size;
                // ####
                //   +----+
                if(o<w->offs && e>w->offs && e<=we) {
                    ptr=malloc(we-o);
                    if(ptr==NULL)
                        return blk;
                    memcpy(ptr,blk,w->offs-o);
                    memcpy(ptr+w->offs-o,w->data,w->size);
                    free(w->data);
                    w->size=we-o;
                    w->offs=o;
                    w->data=ptr;
                    return ptr;
                }
                //     ####
                // +----+
                if(o>w->offs && o<we && e>we) {
                    w->data=realloc(w->data,e-w->offs);
                    if(w->data==NULL)
                        return blk;
                    memcpy(w->data+w->size,blk+we-o,e-we);
                    w->size=e-w->offs;
                    return w->data+o-w->offs;
                }
            }
            // not interfering, then return block as-is
            return blk;

        case FCB_TYPE_DEVICE:
            // reading a block from a device
            if(f->device.inode<ndev) {
                device=&dev[f->device.inode];
                devfs_used(f->device.inode);
                bs=f->device.blksize;
                // in memory device?
                if(device->drivertask==MEMFS_MAJOR) {
                    switch(device->device) {
                        // lsn does not matter for these
                        case MEMFS_NULL:
                            return NULL;
                        case MEMFS_ZERO:
                            zeroblk=(void*)realloc(zeroblk, bs);
                            return zeroblk;
                        case MEMFS_RANDOM:
                            rndblk=(void*)realloc(rndblk, bs);
                            if(rndblk!=NULL)
                                getentropy(rndblk, bs);
                            return rndblk;
                        case MEMFS_RAMDISK:
                            // valid address?
                            if((lsn+1)*bs>f->device.filesize) {
                                seterr(EFAULT);
                                return NULL;
                            }
                            return (void *)(_initrd_ptr + lsn*bs);
                        default:
                            // should never reach this
                            seterr(ENODEV);
                            return NULL;
                    }
                }
                // real device, use block cache
                return cache_getblock(fd, lsn);
            }
            seterr(ENODEV);
            return NULL;

        default:
            break;
    }
    // invalid request, only files and devices support readblock
    seterr(EPERM);
    return NULL;
}

/**
 * write a block to a device with given priority (allowed when device is blocked)
 */
public bool_t writeblock(fid_t fd, blkcnt_t lsn, void *blk, blkprio_t prio)
{
    fcb_t *f;
    devfs_t *device;
    size_t bs;
#if DEBUG
    if(_debug&DBG_BLKIO)
        dbg_printf("FS: writeblock(fd %d, sector %d, buf %x, prio %d)\n",fd,lsn,blk,prio);
#endif
    if(fd==-1 || fd>=nfcb || lsn==-1)
        return false;
    f=&fcb[fd];
    if(!(f->mode & O_WRITE)) { seterr(EACCES); return false; }
    switch(f->type) {
        case FCB_TYPE_REG_FILE:
            // writing a block to a regular file
            if(f->reg.storage==-1 || fcb[f->reg.storage].type!=FCB_TYPE_DEVICE) { seterr(ENODEV); return false; }
            if(fsck.dev==f->reg.storage) { seterr(EBUSY); return false; }
            if(!(fcb[f->reg.storage].mode & O_WRITE) || f->fs >= nfsdrv || fsdrv[f->fs].write==NULL) {
                seterr(EROFS); return false;
            }
            devfs_used(fcb[f->reg.storage].device.inode);
            bs=fcb[f->reg.storage].device.blksize;
            // put it to write buf
            return fcb_write(fd,lsn*bs,blk,bs);

        case FCB_TYPE_DEVICE:
            // writing a block to a device file
            if(f->device.inode<ndev) {
                device=&dev[f->device.inode];
                devfs_used(f->device.inode);
                bs=f->device.blksize;
                // in memory device?
                if(device->drivertask==MEMFS_MAJOR) {
                    switch(device->device) {
                        case MEMFS_NULL:
                            return true;
                        case MEMFS_RAMDISK:
                            // valid address?
                            if((lsn+1)*bs>f->device.filesize) {
                                seterr(EFAULT);
                                return false;
                            }
                            // if it's in place, just return
                            if(blk==(void *)(_initrd_ptr + lsn*bs))
                                return true;
                            // copy the block
                            memcpy((void *)(_initrd_ptr + lsn*bs), blk, bs);
                            return true;

                        default:
                            seterr(EACCES);
                            return false;
                    }
                }
                // real device, use block cache
                return cache_setblock(fd, lsn, blk, prio);
            }
            seterr(ENODEV);
            break;

        default:
            break;
    }
    // invalid request, only files and devices support writeblock
    seterr(EPERM);
    return false;
}

/**
 * return an fcb index for an absolute path. May set errno to EAGAIN on cache miss
 */
fid_t lookup(char *path, bool_t creat)
{
    locate_t loc;
    char *abspath,*tmp=NULL, *c;
    fid_t f,fd,ff,re=0;
    int16_t fs;
    int i,j,l,k;
again:
dbg_printf("lookup '%s' creat %d\n",path,creat);
    if(tmp!=NULL) {
        free(tmp);
        tmp=NULL;
    }
    if(path==NULL || path[0]==0) {
        seterr(EINVAL);
        return -1;
    }
    abspath=canonize(path);
    if(abspath==NULL) {
        seterr(ENOENT);
        return -1;
    }
    // we don't allow joker in path on file creation
    j=strlen(abspath);
    if(creat && j>4) {
        j-=3;
        for(i=0;i<j;i++)
            if(abspath[i]=='.' && abspath[i+1]=='.' && abspath[i+2]=='.') {
                free(abspath);
                seterr(ENOENT);
                return -1;
            }
    }
    f=fcb_get(abspath);
    fd=ff=fs=-1;
    i=j=l=k=0;
    // if not found in cache
    if(f==-1 || (f==ROOTFCB && fcb[f].reg.inode==0)) {
        // first, look at static mounts
        // find the longest match in mtab, root will always match
        for(i=0;i<nmtab;i++) {
            j=strlen(fcb[mtab[i].mountpoint].abspath);
            if(j>l && !memcmp(fcb[mtab[i].mountpoint].abspath, abspath, j)) {
                l=j;
                k=i;
            }
        }
        // get mount device
        fd=mtab[k].storage;
        // get mount point
        ff=mtab[k].mountpoint;
        // and filesystem driver
        fs=mtab[k].fstype;
        // if the longest match was "/", look for automount too
        if(k==ROOTMTAB && !memcmp(abspath, DEVPATH, sizeof(DEVPATH))) {
            i=sizeof(DEVPATH); while(abspath[i]!='/' && !PATHEND(abspath[i])) i++;
            if(abspath[i]=='/') {
                // okay, the path starts with "/dev/*/" Let's get the device fcb
                abspath[i]=0;
                fd=ff=fcb_get(abspath);
                abspath[i++]='/'; l=i;
                fs=-1;
            }
        }
        // only devices and files can serve as storage
        if(fd>=nfcb || (fcb[fd].type!=FCB_TYPE_DEVICE && fcb[fd].type!=FCB_TYPE_REG_FILE)) {
            free(abspath);
            seterr(ENODEV);
            return -1;
        }
        // detect file system on the fly
        if(fs==-1) {
            fs=fsdrv_detect(fd);
            if(k!=ROOTMTAB && fs!=-1)
                mtab[k].fstype=fs;
        }
        // if file system unknown, not much we can do
        // there should be no fsdrv without locate, but be sure
        if(fs==-1 || fsdrv[fs].locate==NULL) {
            free(abspath);
            seterr(ENOFS);
            return -1;
        }
        // check if this device is locked
        if(fsck.dev==fd) {
            free(abspath);
            seterr(EAGAIN);
            return -1;
        }
        // pass the remaining path to filesystem driver
        // fd=device fcb, fs=fsdrv idx, ff=mount point, l=longest mount length
        loc.path=abspath+l;
        loc.inode=-1;
        loc.fileblk=NULL;
        loc.creat=creat;
        pathstackidx=0;
        switch((*fsdrv[fs].locate)(fd, 0, &loc)) {
            case SUCCESS:
                i=strlen(abspath);
                if(loc.type==FCB_TYPE_REG_DIR && i>0 && abspath[i-1]!='/') {
                    abspath[i++]='/'; abspath[i]=0;
                }
                if(f==-1)
                    f=fcb_add(abspath,loc.type);
                fcb[f].fs=fs;
                fcb[f].mode=fcb[fd].mode;   // inherit device's mode
                fcb[f].reg.storage=fd;
                fcb[f].reg.inode=loc.inode;
                fcb[f].reg.filesize=loc.filesize;
                break;
            case NOBLOCK:
                // errno set to EAGAIN on block cache miss and ENOMEM on memory shortage
                break;
            case NOTFOUND:
                seterr(ENOENT);
                break;
            case FSERROR:
                seterr(EBADFS);
                break;
            case UPDIR:
                // TODO: handle up directory
                break;
            case FILEINPATH:
                // TODO: detect file system in image
                break;
            case SYMINPATH:
//dbg_printf("SYMLINK %s %s\n",loc.fileblk,loc.path);
                if(((char*)loc.fileblk)[0]!='/') {
                    // relative symlinks
                    tmp=(char*)malloc(pathmax);
                    if(tmp==NULL) break;
                    c=loc.path-1; while(c>abspath && c[-1]!='/') c--;
                    memcpy(tmp,abspath,c-abspath);
                    tmp[c-abspath]=0;
                    tmp=pathcat(tmp,(char*)loc.fileblk);
                } else {
                    // absolute symlinks
                    tmp=canonize(loc.fileblk);
                }
                if(tmp==NULL) break;
                tmp=pathcat(tmp, loc.path);
                if(tmp==NULL) break;
                path=tmp;
                free(abspath);
                goto again;
            case UNIONINPATH:
                // iterate on union members
                c=(char*)loc.fileblk;
                while(f==-1 && *c!=0 && (c-(char*)loc.fileblk)<__PAGESIZE-1024) {
                    // no unions on file creation
                    if(creat) {
                        seterr(EISUNI);
                        break;
                    }
//dbg_printf("UNION %s %s\n",c,loc.path);
                    // unions must be absolute paths
                    if(*c!='/') {
                        seterr(EBADFS);
                        break;
                    }
                    tmp=canonize(c);
                    if(tmp==NULL) break;
                    tmp=pathcat(tmp, loc.path);
                    if(tmp==NULL) break;
                    f=lookup(tmp, creat);
                    free(tmp);
                    c+=strlen(c)+1;
                }
                break;
            
            case NOSPACE:
#if DEBUG
                if(_debug&DBG_FS)
                    dbg_printf("No space left on device %s (fcb %d)\n", fcb[fd].abspath,fd);
#endif
                // handle initrd a bit differently. Allocate extra memory to it and retry,
                // but only if resizefs() hasn't failed earlier for any reason
                if(fcb[fd].type==FCB_TYPE_DEVICE && dev[fcb[fd].device.inode].drivertask==MEMFS_MAJOR && 
                    dev[fcb[fd].device.inode].device==MEMFS_RAMDISK && fsdrv[fs].resizefs!=NULL && re!=loc.inode &&
                    mmap((void*)(_initrd_ptr+fcb[fd].device.filesize), __PAGESIZE,
                        MAP_PRIVATE|MAP_FIXED|MAP_ANONYMOUS)!=MAP_FAILED) {
                            fcb[fd].device.filesize += __PAGESIZE;
                            free(abspath);
                            (*fsdrv[fs].resizefs)(fd);
                            re=loc.inode;
                            goto again;
                }
                seterr(ENOSPC);
                break;
        }
    }
//dbg_printf("lookup result %s = %d (err %d)\n",abspath,f,errno());
    free(abspath);
    // failsafe
    if(f==-1 && !errno())
        seterr(ENOENT);
    return f;
}

/**
 * read from file. ptr is either a virtual address in ctx->pid's address space (so not
 * writable directly) or in shared memory (writable)
 */
size_t readfs(taskctx_t *tc, fid_t idx, virt_t ptr, size_t size)
{
    size_t bs, ret=0;
    void *blk;
    fcb_t *fc;
    // input checks
    if(!taskctx_validfid(tc,idx))
        return 0;
    fc=&fcb[tc->openfiles[idx].fid];
    switch(fc->type) {
        case FCB_TYPE_PIPE:
            break;

        case FCB_TYPE_SOCKET:
            break;

        default:
            // check if this device is locked
            if(fsck.dev==fc->reg.storage) {
                seterr(EAGAIN);
                return 0;
            }
            if(fc->type==FCB_TYPE_UNION && (tc->openfiles[idx].mode&OF_MODE_READDIR)) {
                if(fc->uni.unionlist[tc->openfiles[idx].unionidx]==0)
                    return 0;
                fc=&fcb[fc->uni.unionlist[tc->openfiles[idx].unionidx]];
            }
            if(fc->fs >= nfsdrv || fsdrv[fc->fs].read==NULL)
                return 0;
            // set up first time run parameters
            if(tc->workleft==-1) {
                tc->workleft=size;
                tc->workoffs=0;
            }
//dbg_printf("readfs storage %d file %d offs %d\n",fc->reg.storage, tc->openfiles[idx].fid, tc->openfiles[idx].offs);
            // read all blocks in
            while(tc->workleft>0) {
                // read max one block
                bs=fcb[fc->reg.storage].device.blksize;
//dbg_printf("readfs %d %d left %d\n",tc->openfiles[idx].offs + tc->workoffs,bs,tc->workleft);
                // call file system driver
                blk=(*fsdrv[fc->fs].read)(
                    fc->reg.storage, fc->reg.inode, tc->openfiles[idx].offs + tc->workoffs, &bs);
//dbg_printf("readfs ret %x bs %d, delayed %d\n",blk,bs,ackdelayed);
                // skip if block is not in cache
                if(ackdelayed) break;
                // if eof
                if(bs==0) {
                    ret = tc->workoffs;
                    tc->workleft=-1;
                    break;
                } else if(blk==NULL) {
                    // spare portion of file? return zero block
                    zeroblk=(void*)realloc(zeroblk, fcb[fc->reg.storage].device.blksize);
                    blk=zeroblk;
                }
                // failsafe, get number of bytes read
                if(bs>=tc->workleft) { bs=tc->workleft; tc->workleft=-1; ret=tc->workoffs+bs; } else tc->workleft-=bs;
                // copy result to caller. If shared memory, directly; otherwise via a core syscall
                if((int64_t)ptr < 0)
                    memcpy((void*)(ptr + tc->workoffs), blk, bs);
                else
                    p2pcpy(tc->pid, (void*)(ptr + tc->workoffs), blk, bs);
                tc->workoffs+=bs;
            }
            // only alter seek offset when read is finished and it's nor readdir
            if(!(tc->openfiles[idx].mode&OF_MODE_READDIR))
                tc->openfiles[idx].offs += ret;
            break;
    }
    return ret;
}

/**
 * write to file. ptr is either a buffer in message queue or shared memory, so it's readable
 * in both cases
 */
size_t writefs(taskctx_t *tc, fid_t idx, void *ptr, size_t size)
{
    fcb_t *fc;
    // input checks
    if(!taskctx_validfid(tc,idx))
        return 0;
    fc=&fcb[tc->openfiles[idx].fid];
    switch(fc->type) {
        case FCB_TYPE_PIPE:
            break;

        case FCB_TYPE_SOCKET:
            break;

        default:
            // check if this device is locked
            if(fsck.dev==fc->reg.storage) {
                seterr(EAGAIN);
                return 0;
            }
            if(fc->fs>=nfsdrv)
                return 0;
            // set up first time run parameters
            if(tc->workleft==-1) {
                tc->workleft=size;
                tc->workoffs=0;
            }
            break;
    }
    return 0;
}

/**
 * return a stat_t structure for file
 */
stat_t *statfs(fid_t idx)
{
    // failsafe
    if(idx>=nfcb || fcb[idx].abspath==NULL)
        return NULL;
    // check if this device is locked
    if(fsck.dev==fcb[idx].reg.storage) {
        seterr(EAGAIN);
        return 0;
    }
    memzero(&st,sizeof(stat_t));
//dbg_printf("statfs(%d)\n",idx);
    // add common fcb fields
    st.st_dev=fcb[idx].reg.storage;
    st.st_ino=fcb[idx].reg.inode;
    st.st_mode=(fcb[idx].mode&0xFFFF);
    if(fcb[idx].type==FCB_TYPE_UNION)
        st.st_mode|=S_IFDIR|S_IFUNI;
    else
        st.st_mode|=(fcb[idx].type << 16);
    st.st_size=fcb[idx].reg.filesize;
    // type specific fields
    switch(fcb[idx].type) {
        case FCB_TYPE_DEVICE:
            st.st_blksize=fcb[idx].device.blksize;
            if(fcb[idx].device.blksize==1)
                st.st_mode|=S_IFCHR;
            break;
        case FCB_TYPE_PIPE:
            break;
        case FCB_TYPE_SOCKET:
            break;
        default:
            // call file system driver to fill in various stat_t fields
            if(fcb[idx].fs>=nfsdrv || fsdrv[fcb[idx].fs].stat==NULL ||
                !(*fsdrv[fcb[idx].fs].stat)(fcb[idx].reg.storage, fcb[idx].reg.inode, &st))
                return NULL;
            st.st_blksize=fcb[fcb[idx].reg.storage].device.blksize;
            break;
    }
    return &st;
}

/**
 * read the next directory entry
 */
dirent_t *readdirfs(taskctx_t *tc, fid_t idx, void *ptr, size_t size)
{
    fcb_t *fc;
    size_t s;
    fid_t f;
    char *abspath;
    // failsafe
    if(!taskctx_validfid(tc,idx))
        return NULL;
    fc=&fcb[tc->openfiles[idx].fid];
    if(fc->fs >= nfsdrv || fsdrv[fc->fs].getdirent==NULL)
        return 0;
    // check if this device is locked
    if(fsck.dev==fc->reg.storage) {
        seterr(EAGAIN);
        return 0;
    }
    memzero(&dirent,sizeof(dirent_t));
    // call file system driver to parse the directory entry
    s=(*fsdrv[fc->fs].getdirent)(ptr, tc->openfiles[idx].offs, &dirent);
    // failsafe
    if(s==0 || dirent.d_ino==0 || dirent.d_name[0]==0)
        return NULL;
    if(dirent.d_len==0)
        dirent.d_len=strlen(dirent.d_name);
    dirent.d_dev=fc->reg.storage;
    dirent.d_filesize=fc->reg.filesize;
    // assume regular file
    dirent.d_type=FCB_TYPE_REG_FILE;
    // call file system driver to fill in st_mode stat_t field
    memzero(&st,sizeof(stat_t));
    if(fsdrv[fc->fs].stat!=NULL && (*fsdrv[fc->fs].stat)(fc->reg.storage, dirent.d_ino, &st)) {
        dirent.d_type=st.st_mode>>16;
        // if it's a symlink, get the target's type and size
        if(lastlink!=NULL) {
            f=-1;
            if(lastlink[0]=='/') {
                f=lookup(lastlink, false);
            } else {
                abspath=(char*)malloc(pathmax);
                if(abspath!=NULL) {
                    strcpy(abspath, fc->type==FCB_TYPE_UNION ?
                            fcb[fc->uni.unionlist[tc->openfiles[idx].unionidx]].abspath : fc->abspath);
                    pathcat(abspath,lastlink);
                    f=lookup(abspath, false);
                    free(abspath);
                }
            }
            if(ackdelayed) return NULL;
            if(f!=-1 && fcb[f].fs<nfsdrv && (*fsdrv[fcb[f].fs].stat)(fcb[f].reg.storage, fcb[f].reg.inode, &st)) {
                    dirent.d_type &= ~(S_IFMT>>16);
                    dirent.d_type |= st.st_mode>>16;
                    dirent.d_filesize=st.st_size;
                    // let it copy the icon from the updated stat_t buf
            }
        }
        memcpy(&dirent.d_icon,&st.st_type,8);
        if(dirent.d_icon[3]==':')
            dirent.d_icon[3]=0;
    }
    // move open file pointer
    tc->openfiles[idx].offs += s;
    if(fc->type==FCB_TYPE_UNION && tc->openfiles[idx].offs >=
        fcb[fc->uni.unionlist[tc->openfiles[idx].unionidx]].uni.filesize) {
        tc->openfiles[idx].offs = 0;
        tc->openfiles[idx].unionidx++;
    }
    return &dirent;
}

/**
 * do a file system check on a device
 */
bool_t dofsck(fid_t fd, bool_t fix)
{
    // failsafe
    if(fd==-1 || fsck.dev!=-1 || fsck.dev!=fd || fcb[fd].type!=FCB_TYPE_DEVICE || fcb[fd].device.storage!=DEVFCB)
        return false;
    // first call to dofsck?
    if(fsck.dev==-1) {
        fsck.dev=fd;
        fsck.fix=fix;
        fsck.fs=fsdrv_detect(fd);
    }
    if(fsck.fs!=-1 && fsck.fs<nfsdrv && fsdrv[fsck.fs].checkfs!=NULL) {
        // continue the step that was interrupted
        while(fsck.step<FSCKSTEP_DONE) {
            if(!(*fsdrv[fsck.fs].checkfs)(fd))
                return false;
            fsck.step++;
        }
    }
    // clear lock and reset step counter
    fsck.dev=-1;
    fsck.step=0;
    return true;
}
