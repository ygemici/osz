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
 * @brief Virtual File System functions
 */

#include <osZ.h>
#include <sys/driver.h>
#include "fcb.h"
#include "vfs.h"
#include "cache.h"
#include "devfs.h"
#include "mtab.h"
#include "taskctx.h"

extern uint8_t ackdelayed;      // flag to indicate async block read
extern uint32_t _pathmax;       // max length of path
extern uint64_t _initrd_ptr;    // /dev/root pointer and size
extern uint64_t _initrd_size;

void *zeroblk = NULL;
void *rndblk = NULL;

int pathstackidx = 0;
ino_t pathstack[PATHSTACKSIZE];

/**
 * add an inode reference to path stack. Rotate if stack grows too big
 */
public void pathpush(ino_t lsn)
{
    if(pathstackidx==PATHSTACKSIZE) {
        memcpy(&pathstack[0], &pathstack[1], (PATHSTACKSIZE-1)*sizeof(ino_t));
        pathstack[PATHSTACKSIZE-1] = lsn;
        return;
    }
    pathstack[pathstackidx++] = lsn;
}

/**
 * pop the last inode from path stack
 */
public ino_t pathpop()
{
    if(pathstackidx==0)
        return 0;
    pathstackidx--;
    return pathstack[pathstackidx];
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
    if(i+strlen(filename)>=_pathmax)
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
char *canonize(const char *path, char *result)
{
    int i=0,j=0,k,l=false,m;
    if(path==NULL || path[0]==0)
        return NULL;
    if(result==NULL) {
        result=(char*)malloc(_pathmax);
        if(result==NULL)
            return NULL;
        l=true;
    }
    k=strlen(path);
    // translate dev: paths
    while(i<k && path[i]!=':' && path[i]!='/' && !PATHEND(path[i])) i++;
    if(path[i]==':') {
        strcpy(result, DEVPATH); j=sizeof(DEVPATH);
        strncpy(result+j, path, i); j+=i;
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
        if(path[i]=='.' && i+1<k && path[i+1]=='/') {
            i+=2; continue;
/*
        // do not handle directory up here, as last directory in result
        // could be a symlink
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
    if(l) {
        // no need to shrink memory as this buffer will be freed soon enough
//        result=(char*)realloc(result, j+1);
    }
    return result;
}

/**
 * read a block from an fcb entry
 */
public void *readblock(fid_t fd, blkcnt_t offs, blksize_t bs)
{
    devfs_t *device;
    void *blk=NULL;
    // failsafe
    if(fd>=nfcb)
        return NULL;

    switch(fcb[fd].type) {
        case FCB_TYPE_REG_FILE: 
            //TODO: reading a block from a regular file
            break;
        case FCB_TYPE_DEVICE:
            // reading a block from a device file
            if(fcb[fd].device.inode<ndev) {
                device=&dev[fcb[fd].device.inode];
                // in memory device?
                if(device->drivertask==MEMFS_MAJOR) {
                    switch(device->device) {
                        case MEMFS_NULL:
                            //eof?
                            return NULL;
                        case MEMFS_ZERO:
                            zeroblk=(void*)realloc(zeroblk, bs);
                            return zeroblk;
                        case MEMFS_RANDOM:
                            rndblk=(void*)realloc(rndblk, bs);
                            if(rndblk!=NULL)
                                getentropy(rndblk, bs);
                            return rndblk;
                        case MEMFS_TMPFS:
                            // TODO: implement tmpfs memory device
                            return NULL;
                        case MEMFS_RAMDISK:
                            if((offs+1)*device->blksize>_initrd_size) {
                                seterr(ENXIO);
                                return NULL;
                            }
                            return (void *)(_initrd_ptr + offs*device->blksize);
                        default:
                            // should never reach this
                            return NULL;
                    }
                }
                // real device, use block cache
                seterr(SUCCESS);
                blk=cache_getblock(fcb[fd].device.inode, offs);
                if(blk==NULL && errno()==EAGAIN) {
                    // block not found in cache. Send a message to a driver
                    mq_send(device->drivertask, DRV_read, device->device, offs, ctx->pid, 0);
                    // delay ack message to the original caller
                    ackdelayed = true;
                }
            }
            break;
        default:
            // invalid request
            break;
    }
    return blk;
}

/**
 * return an fcb index for an absolute path. May set errno to EAGAIN on cache miss
 */
fid_t lookup(char *path)
{
    if(path==NULL || path[0]==0)
        return -1;
    locate_t loc;
    char *abspath=canonize(path,NULL);
    fid_t f=fcb_get(abspath),fd=-1,ff=-1;
    int16_t fs=-1;
    int i,j,l=0,k=0;
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
/*dbg_printf("locate '%s': '%s' mp '%s' fstype '%s' path '%s'\n",
  abspath,fcb[fd].abspath,fcb[mtab[k].mountpoint].abspath,fsdrv[fs].name,abspath+l);*/
        loc.path=abspath+l;
        loc.result=-1;
        loc.fileblk=NULL;
        switch((*fsdrv[fs].locate)(fd, &loc)) {
            case SUCCESS:
                f=loc.result;
                break;
            case NOBLOCK:
                // errno set to EAGAIN on block cache miss
                f=-1;
                break;
            case NOTFOUND:
                seterr(ENOENT);
                f=-1;
                break;
            case FSERROR:
                seterr(ENOFS);
                f=-1;
                break;
            case UPDIR:
                break;
            case FILEINPATH:
                break;
            case SYMINPATH:
                break;
            case UNIONINPATH:
                break;
        }
    }
    free(abspath);
    return f;
}
