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

extern uint8_t ackdelayed;      // flag to indicate async block read
extern uint32_t pathmax;        // max length of path
extern uint64_t _initrd_ptr;    // /dev/root pointer and size
extern uint64_t _initrd_size;

void *zeroblk = NULL;           // zero block
void *rndblk = NULL;            // random data block

stat_t st;                      // buffer for stat() and fstat()

int pathstackidx = 0;           // path stack
pathstack_t pathstack[PATHSTACKSIZE];

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
    // trailing slash
    if(path[i-1]=='/')
        result[j++]='/';
    result[j]=0;
    return result;
}

/**
 * read a block from an fcb entry, offs is a LSN
 */
public void *readblock(fid_t fd, blkcnt_t lsn)
{
    devfs_t *device;
    void *blk=NULL;
    size_t bs;
    // failsafe
    if(fd==-1 || fd>=nfcb || lsn==-1)
        return NULL;
#if DEBUG
    if(_debug&DBG_BLKIO)
        dbg_printf("FS: readblock(fd %d, sector %d)\n",fd,lsn);
#endif
    switch(fcb[fd].type) {
        case FCB_TYPE_REG_FILE:
            // reading a block from a regular file
            if(fcb[fd].reg.storage==-1 || fcb[fd].reg.fs==-1 || fcb[fd].reg.fs >= nfsdrv)
                return NULL;
            bs=fcb[fcb[fd].reg.storage].device.blksize;
            // call file system driver to read block
            blk=(*fsdrv[fcb[fd].reg.fs].read)(fcb[fd].reg.storage, fcb[fd].reg.inode, lsn*bs, &bs);
            // skip if block is not in cache or eof
            if(ackdelayed || bs==0) return NULL;
            if(blk==NULL) {
                // spare portion of file? return zero block
                zeroblk=(void*)realloc(zeroblk, fcb[fcb[fd].reg.storage].device.blksize);
                return zeroblk;
            }
            return blk;
            break;
        case FCB_TYPE_PIPE:
            break;
        case FCB_TYPE_DEVICE:
            // reading a block from a device file
            if(fcb[fd].device.inode<ndev) {
                device=&dev[fcb[fd].device.inode];
                // in memory device?
                if(device->drivertask==MEMFS_MAJOR) {
                    switch(device->device) {
                        case MEMFS_NULL:
                            return NULL;
                        case MEMFS_ZERO:
                            zeroblk=(void*)realloc(zeroblk, fcb[fd].device.blksize);
                            return zeroblk;
                        case MEMFS_RANDOM:
                            rndblk=(void*)realloc(rndblk, fcb[fd].device.blksize);
                            if(rndblk!=NULL)
                                getentropy(rndblk, fcb[fd].device.blksize);
                            return rndblk;
                        case MEMFS_RAMDISK:
                            if((lsn+1)*fcb[fd].device.blksize>_initrd_size) {
                                return NULL;
                            }
                            return (void *)(_initrd_ptr + lsn*fcb[fd].device.blksize);
                        default:
                            // should never reach this
                            seterr(ENODEV);
                            return NULL;
                    }
                }
                // real device, use block cache
                if((lsn+1)*fcb[fd].device.blksize>fcb[fd].device.filesize) {
                    return NULL;
                }
                seterr(SUCCESS);
                blk=cache_getblock(fd, lsn);
                if(blk==NULL && errno()==EAGAIN) {
                    // block not found in cache. Send a message to a driver
                    mq_send(device->drivertask, DRV_read, device->device, lsn, fcb[fd].device.blksize, ctx->pid, 0);
                    // delay ack message to the original caller
                    ackdelayed = true;
                }
            }
            break;
        default:
            // invalid request
            seterr(EPERM);
            break;
    }
    return blk;
}

/**
 * return an fcb index for an absolute path. May set errno to EAGAIN on cache miss
 */
fid_t lookup(char *path, bool_t creat)
{
    locate_t loc;
    char *abspath,*tmp=NULL, *c;
    fid_t f,fd,ff;
    int16_t fs;
    int i,j,l,k;
again:
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
        seterr(EINVAL);
        return -1;
    }
    f=fcb_get(abspath);
    fd=ff=fs=-1;
    i=j=l=k=0;
    // if not found in cache
    if(f==-1) {
        // first, look at static mounts
        // find the longest match in mtab, root fill always match
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
        if(fs==-1 || fsdrv[fs].locate==NULL) {
            free(abspath);
            seterr(ENOFS);
            return -1;
        }
        // pass the remaining path to filesystem driver
        // fd=device fcb, fs=fsdrv idx, ff=mount point, l=longest mount length
        loc.path=abspath+l;
        loc.inode=f=-1;
        loc.fileblk=NULL;
        loc.creat=creat;
        pathstackidx=0;
        switch((*fsdrv[fs].locate)(fd, 0, &loc)) {
            case SUCCESS:
                f=strlen(abspath);
                if(loc.type==FCB_TYPE_REG_DIR && abspath[f-1]!='/') {
                    abspath[f++]='/'; abspath[f]=0;
                }
                f=fcb_add(abspath,loc.type);
                fcb[f].reg.storage=fd;
                fcb[f].reg.inode=loc.inode;
                fcb[f].reg.filesize=loc.filesize;
                fcb[f].reg.fs=fs;
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
                break;
            case FILEINPATH:
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
 * read from file. ptr is either a virtual address in ctx->pid's address space (so not
 * writable directly) or in shared memory (writable)
 */
public size_t readfs(taskctx_t *tc, fid_t idx, virt_t ptr, size_t size)
{
    size_t bs, ret=0;
    void *blk;
    fcb_t *fc;
    // input checks
    if(!taskctx_validfid(tc,idx))
        return 0;
    fc=&fcb[tc->openfiles[idx].fid];
    if(fc->reg.fs==-1 || fc->reg.fs >= nfsdrv)
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
        blk=(*fsdrv[fc->reg.fs].read)(
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
    // only alter seek offset when read is finished
    tc->openfiles[idx].offs += ret;
    return ret;
}

/**
 * write to file. ptr is either a buffer in message queue or shared memory, so it's readable
 * in both cases
 */
public size_t writefs(taskctx_t *tc, fid_t idx, void *ptr, size_t size)
{
    fcb_t *fc;
    // input checks
    if(!taskctx_validfid(tc,idx))
        return 0;
    fc=&fcb[tc->openfiles[idx].fid];
    if(fc->reg.fs==-1)
        return 0;
    // set up first time run parameters
    if(tc->workleft==-1) {
        tc->workleft=size;
        tc->workoffs=0;
    }
    return 0;
}

/**
 * return a stat structure for file
 */
public stat_t *statfs(fid_t idx)
{
    // failsafe
    if(idx>=nfcb || fcb[idx].abspath==NULL)
        return NULL;
    memset(&st,0,sizeof(stat_t));
    // add common fcb fields
    st.st_dev=fcb[idx].reg.storage;
    st.st_ino=fcb[idx].reg.inode;
    st.st_mode|=(fcb[idx].mode&0xFFFF) | (fcb[idx].type << 16);
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
            if(fcb[idx].reg.fs==-1 || fcb[idx].reg.fs>=nfsdrv || 
                !(*fsdrv[fcb[idx].reg.fs].stat)(fcb[idx].reg.storage, fcb[idx].reg.inode, &st))
                return NULL;
            st.st_blksize=fcb[fcb[idx].reg.storage].device.blksize;
            break;
    }
    return &st;
}
