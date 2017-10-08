/*
 * fs/taskctx.c
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
 * @brief Task context for file system services
 */

#include <osZ.h>
#include "vfs.h"

/* task contexts */
uint64_t ntaskctx = 0;
taskctx_t *taskctx[256];

/* current context */
public taskctx_t *ctx;

/**
 * get a task context (add a new one if not found)
 */
taskctx_t *taskctx_get(pid_t pid)
{
    taskctx_t *tc=taskctx[pid & 0xFF], *ntc;
    // find in hash
    while(tc!=NULL) {
        if(tc->pid==pid)
            return tc;
        tc=tc->next;
    }
    // not found, add
    ntc=(taskctx_t*)malloc(sizeof(taskctx_t));
    if(ntc==NULL)
        return NULL;
    ntc->pid=pid;
    // link
    if(tc!=NULL)
        tc->next=ntc;
    else
        taskctx[pid & 0xFF]=ntc;
    fcb[0].nopen+=2;
    ntaskctx++;
    return ntc;
}

/**
 * remove a task context
 */
void taskctx_del(pid_t pid)
{
    taskctx_t *tc=taskctx[pid & 0xFF], *ltc=NULL;
    uint64_t i;
    // failsafe
    if(tc==NULL)
        return;
    // find in hash
    while(tc->next!=NULL) {
        if(tc->pid==pid)
            break;
        ltc=tc;
        tc=tc->next;
    }
    // unlink
    if(ltc==NULL)
        taskctx[pid & 0xFF]=tc->next;
    else
        ltc->next=tc->next;
    ntaskctx--;
    // free memory
    fcb_del(tc->workdir);
    fcb_del(tc->rootdir);
    if(tc->openfiles!=NULL) {
        for(i=0;i<tc->nopenfiles;i++)
            fcb_del(tc->openfiles[i].fid);
        free(tc->openfiles);
    }
    free(tc);
}

/**
 * set root directory
 */
void taskctx_rootdir(taskctx_t *tc, fid_t fid)
{
    if(tc==NULL || fid>=nfcb || fcb[fid].abspath==NULL)
        return;
    fcb_del(tc->rootdir);
    tc->rootdir=fid;
    fcb[fid].nopen++;
}

/**
 * set working directory
 */
void taskctx_workdir(taskctx_t *tc, fid_t fid)
{
    if(tc==NULL || fid>=nfcb || fcb[fid].abspath==NULL)
        return;
    fcb_del(tc->workdir);
    tc->workdir=fid;
    fcb[fid].nopen++;
}

/**
 * add a file to open file descriptors
 */
uint64_t taskctx_open(taskctx_t *tc, fid_t fid, mode_t mode, fpos_t offs, uint64_t idx)
{
    uint64_t i;
    fcb_t *f;
    openfiles_t *of;
    if(tc==NULL || fid>=nfcb || fcb[fid].abspath==NULL) {
        seterr(ENOENT);
        return -1;
    }
    f=&fcb[fid];
    // check if it's already opened for exclusive access
    if(f->flags & FCB_FLAG_EXCL) {
        seterr(EBUSY);
        return -1;
    }
    // Check if the requested operation permitted on the device
    if(!(mode & O_READ) && !(mode & O_WRITE))
        mode |= O_READ;
    // Matches flags, or needs creat or append and has write permission
    if((mode & O_AMSK) != (f->mode & O_AMSK) && !((mode & (O_APPEND|O_CREAT)) && (f->mode & O_WRITE))) {
        seterr(EACCES);
        return -1;
    }
    if((mode & O_CREAT) || (mode & O_APPEND))
        mode |= O_WRITE;
    // force file descriptor index
    if(idx!=-1 && idx<tc->nopenfiles && tc->openfiles[idx].fid==-1) {
        i=idx;
        goto found;
    }
    // find the first empty slot
    if(tc->openfiles!=NULL) {
        for(i=0;i<tc->nopenfiles;i++)
            if(tc->openfiles[i].fid==-1)
                goto found;
    } else
        tc->nopenfiles=0;
    // if not found, allocate a new slot
    i=tc->nopenfiles;
    tc->nopenfiles++;
    tc->openfiles=(openfiles_t*)realloc(tc->openfiles,tc->nopenfiles*sizeof(openfiles_t));
    if(tc->openfiles==NULL)
        return -1;
    // add new open file descriptor
found:
    of=&tc->openfiles[i];
    // handle exclusive access. Either user asked for it
    // or it's a special device
    if((mode & O_EXCL) || (f->mode & O_EXCL))
        f->flags |= FCB_FLAG_EXCL;
    // save open file descriptor
    of->fid=fid;
    of->mode=mode;
    of->offs=offs;
    of->unionidx=0;
    tc->nfiles++;
    f->nopen++;
    return i;
}

/**
 * remove a file from open file descriptors
 */
bool_t taskctx_close(taskctx_t *tc, uint64_t idx, bool_t dontfree)
{
    fcb_t *f;
    openfiles_t *of;
    if(!taskctx_validfid(tc,idx))
        return false;
    of=&tc->openfiles[idx];
    f=&fcb[of->fid];
    if(of->mode & O_WRITE)
        fcb_flush(of->fid);
    if(of->mode & O_TMPFILE && f->mode!=FCB_TYPE_REG_DIR) {
        //unlink()
    } else {
        f->flags &= ~FCB_FLAG_EXCL;
        fcb_del(of->fid);
    }
    of->fid=-1;
    tc->nfiles--;
    if(dontfree)
        return true;
    idx=tc->nopenfiles;
    while(idx>0 && tc->openfiles[idx-1].fid==-1) idx--;
    if(idx!=tc->nopenfiles) {
        tc->nopenfiles=idx;
        if(idx==0) {
            free(tc->openfiles);
            tc->openfiles=NULL;
        } else {
            tc->openfiles=(openfiles_t*)realloc(tc->openfiles,tc->nopenfiles*sizeof(openfiles_t));
            if(tc->openfiles==NULL)
                return false;
        }
    }
    return true;
}

/**
 * change position on a open file descriptor
 */
bool_t taskctx_seek(taskctx_t *tc, uint64_t idx, off_t offs, uint8_t whence)
{
    openfiles_t *of;
    if(!taskctx_validfid(tc,idx)){
        seterr(EBADF);
        return false;
    }
    of=&tc->openfiles[idx];
    if(fcb[of->fid].type!=FCB_TYPE_REG_FILE) {
        seterr(ESPIPE);
        return false;
    }
    switch(whence) {
        case SEEK_CUR:
            of->offs += offs;
            break;
        case SEEK_END:
            of->offs = fcb[of->fid].reg.filesize + offs;
            break;
        default:
            of->offs = offs;
            break;
    }
    if((off_t)of->offs<0) {
        of->offs = 0;
        seterr(EINVAL);
        return false;
    }
    return true;
}

/**
 * see if a file descriptor valid for a context
 */
bool_t taskctx_validfid(taskctx_t *tc, uint64_t idx)
{
    return !(tc==NULL || tc->openfiles==NULL || tc->nopenfiles<=idx || tc->openfiles[idx].fid==-1 ||
        fcb[tc->openfiles[idx].fid].abspath==NULL);
}

/**
 * read the next directory entry
 */
dirent_t *taskctx_readdir(taskctx_t *tc, fid_t idx, void *ptr, size_t size)
{
    fcb_t *fc;
    size_t s;
    fid_t f;
    openfiles_t *of;
    char *abspath;
    // failsafe
    if(!taskctx_validfid(tc,idx))
        return NULL;
    of=&tc->openfiles[idx];
    fc=&fcb[of->fid];
    if(fc->fs >= nfsdrv || fsdrv[fc->fs].getdirent==NULL) {
        seterr(ENOTSUP);
        return 0;
    }
    // check if this device is locked
    if(fsck.dev==fc->reg.storage) {
        seterr(EBUSY);
        return 0;
    }
    memzero(&dirent,sizeof(dirent_t));
    // call file system driver to parse the directory entry
    s=(*fsdrv[fc->fs].getdirent)(ptr, of->offs, &dirent);
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
                            fcb[fc->uni.unionlist[of->unionidx]].abspath : fc->abspath);
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
    of->offs += s;
    if(fc->type==FCB_TYPE_UNION && of->offs >= fcb[fc->uni.unionlist[of->unionidx]].uni.filesize) {
        of->offs = 0;
        of->unionidx++;
    }
    return &dirent;
}

/**
 * read from file. ptr is either a virtual address in ctx->pid's address space (so not
 * writable directly) or in shared memory (writable)
 */
size_t taskctx_read(taskctx_t *tc, fid_t idx, virt_t ptr, size_t size)
{
    size_t bs, ret=0;
    void *blk,*p;
    fcb_t *f;
    openfiles_t *of;
    writebuf_t *w;
    off_t o,e,we;
    // input checks
    if(!taskctx_validfid(tc,idx)) {
        seterr(EBADF);
        return 0;
    }
    of=&tc->openfiles[idx];
    f=&fcb[of->fid];
    if(!(f->mode & O_READ)) { seterr(EACCES); return 0; }
    switch(f->type) {
        case FCB_TYPE_PIPE:
            break;

        case FCB_TYPE_SOCKET:
            break;

        default:
            // check if this device is locked
            if(f->reg.storage!=-1 && fsck.dev==f->reg.storage) { seterr(EBUSY); return 0; }
            if(f->type==FCB_TYPE_UNION && (of->mode & OF_MODE_READDIR)) {
                if(f->uni.unionlist[of->unionidx]==0)
                    return 0;
                f=&fcb[f->uni.unionlist[of->unionidx]];
            }
            if(f->fs >= nfsdrv || fsdrv[f->fs].read==NULL) { seterr(EACCES); return 0; }
            // check if the read can be served from write buf entirely
            if(tc->workleft==(size_t)-1) {
                e=of->offs+size;
                for(w=f->reg.buf;w!=NULL;w=w->next) {
                    //     ####
                    //  +--------+
                    if(of->offs>=w->offs && e<=w->offs+w->size) {
                        if((int64_t)ptr < 0)
                            memcpy((void*)ptr, w->data+of->offs-w->offs, size);
                        else
                            p2pcpy(tc->pid, (void*)ptr, w->data+of->offs-w->offs, size);
                        return size;
                    }
                }
                // no, we have to split it up into blocks and serve one by one
                // set up first time run parameters
                tc->workleft=size; tc->workoffs=0;
            }
//dbg_printf("readfs storage %d file %d offs %d\n",fc->reg.storage, tc->openfiles[idx].fid, tc->openfiles[idx].offs);
            // read all blocks in
            while(tc->workleft>0) {
                // read max one block
                bs=fcb[f->reg.storage].device.blksize;
//dbg_printf("readfs %d %d left %d\n",tc->openfiles[idx].offs + tc->workoffs,bs,tc->workleft);
                blk=NULL;
                // let's see if one block can be served from write buf
                o=of->offs + tc->workoffs; e=o+bs;
                for(w=f->reg.buf;w!=NULL;w=w->next) {
                    //     ####
                    //  +--------+
                    if(o>=w->offs && e<=w->offs+w->size) {
                        blk=w->data+o-w->offs;
                        break;
                    }
                }
                if(blk==NULL) {
                    // call file system driver
                    blk=(*fsdrv[f->fs].read)(f->reg.storage, f->reg.inode, o, &bs);
//dbg_printf("readfs ret %x bs %d, delayed %d\n",blk,bs,ackdelayed);
                    // skip if block is not in cache
                    if(ackdelayed) break;
                    // if eof
                    if(bs==0) {
                        ret = tc->workoffs; tc->workleft=-1; break;
                    } else if(blk==NULL) {
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
                            p=malloc(we-o);
                            if(p==NULL) break;
                            memcpy(p,blk,w->offs-o); memcpy(p+w->offs-o,w->data,w->size);
                            free(w->data); w->size=we-o; w->offs=o; w->data=p; blk=p;
                            break;
                        }
                        //     ####
                        // +----+
                        if(o>w->offs && o<we && e>we) {
                            w->data=realloc(w->data,e-w->offs);
                            if(w->data==NULL) break;
                            memcpy(w->data+w->size,blk+we-o,e-we);
                            w->size=e-w->offs; blk=w->data+o-w->offs;
                            break;
                        }
                    }
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
            // only alter seek offset when read is finished and it's not readdir
            if(!(of->mode & OF_MODE_READDIR))
                of->offs += ret;
            break;
    }
    return ret;
}

/**
 * write to file. ptr is either a buffer in message queue or shared memory, so it's readable
 * in both cases
 */
size_t taskctx_write(taskctx_t *tc, fid_t idx, void *ptr, size_t size)
{
    fcb_t *f;
    openfiles_t *of;
    // input checks
    if(!taskctx_validfid(tc,idx)) {
        seterr(EBADF);
        return 0;
    }
    of=&tc->openfiles[idx];
    f=&fcb[of->fid];
    if(!(f->mode & O_WRITE)) { seterr(EACCES); return 0; }
    switch(f->type) {
        case FCB_TYPE_PIPE:
            break;

        case FCB_TYPE_SOCKET:
            break;

        case FCB_TYPE_DEVICE:
            break;

        case FCB_TYPE_REG_DIR:
        case FCB_TYPE_UNION:
            // should never reach this, but failsafe
            seterr(EPERM);
            break;

        default:
            if(fcb_write(of->fid, of->offs, ptr, size)) {
                of->offs += size;
                return size;
            }
            break;
    }
    return 0;
}


#if DEBUG
void taskctx_dump()
{
    int i,j;
    taskctx_t *tc;
    dbg_printf("Task Contexts %d:\n",ntaskctx);
    for(i=0;i<256;i++) {
        if(taskctx[i]==NULL)
            continue;
        tc=taskctx[i];
        while(tc!=NULL) {
            dbg_printf(" %x %s%02x ", tc, ctx==tc?"*":" ", tc->pid);
            dbg_printf("root %3d pwd %3d open %2d:", tc->rootdir, tc->workdir, tc->nopenfiles);
            for(j=0;j<tc->nopenfiles;j++)
                dbg_printf(" (%d,%1x,%d)", tc->openfiles[j].fid, tc->openfiles[j].mode, tc->openfiles[j].offs);
            dbg_printf("\n");
            tc=tc->next;
        }
    }
}
#endif
