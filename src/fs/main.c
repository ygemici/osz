/*
 * fs/main.c
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
 * @brief File System Service
 */

#include <osZ.h>
#include <sys/driver.h>
#include "cache.h"
#include "mtab.h"
#include "devfs.h"
#include "vfs.h"

public uint8_t *_initrd_ptr=NULL;   // initrd offset in memory (filled in by real-time linker)
public uint64_t _initrd_size=0;     // initrd size in bytes (filled in by real-time linker)
public char *_fstab_ptr=NULL;       // pointer to /etc/fstab data (filler in by real-time linker)
public size_t _fstab_size=0;        // fstab size (filled in by real-time linker)
public uint32_t pathmax;            // maximum length of path
public uint32_t cachelines;         // number of block cache lines
public uint32_t cachelimit;         // if free RAM drops below this percentage, free up half of cache
bool_t ackdelayed;                  // flag to indicate async block read 
bool_t nomem=false;                 // flag to indicate cache needs to be freed

/**
 * File System task main procedure
 */
void _init(int argc, char **argv)
{
    msg_t *msg;
    uint64_t ret = 0, j, k;
    off_t o;
    void *ptr;
    meminfo_t m;

    // environment
    pathmax    = env_num("pathmax",512,256,1024*1024);
    cachelines = env_num("cachelines",16,16,65535);
    cachelimit = env_num("cachelimit",5,1,50);

    // initialize vfs structures
    mtab_init();    // registers "/" as first fcb
    devfs_init();   // registers "/dev/" as second fcb
    cache_init();
    fsck.dev=-1;    // no locked device at start
    nomem=false;    // we have memory at start

    /*** endless loop ***/
    while(1) {
        /* get work */
        msg = mq_recv();
        // clear state
        seterr(SUCCESS);
        // suspend all messages in a nomem situation, otherwise
        // only if it requires an ack message from a driver task
        ackdelayed = nomem;
        ret=0;
        // if it's a block ready message
        if(EVT_FUNC(msg->evt) == SYS_setblock) {
            // save it to block cache
            cache_setblock(msg->attr0/*fcbdev*/, msg->attr1/*offs*/, msg->ptr/*blk*/, BLKPRIO_NOTDIRTY);
            // resume original message that caused it. Don't use taskctx_get here,
            // as that would always return a valid context
            ctx=taskctx[msg->arg4 & 0xFF]; if(ctx==NULL) continue;
            while(ctx!=NULL) { if(ctx->pid==msg->arg4) break; ctx=ctx->next; }
            if(ctx==NULL || EVT_FUNC(ctx->msg.evt)==0) continue;
            // ok, msg->arg4 points to a pid with a valid context with a suspended message
            msg = &ctx->msg;
        } else
        // if it's a block written out acknowledge
        if(EVT_FUNC(msg->evt) == SYS_ackblock) {
            // failsafe, no block count and cache for character devices
            if(msg->arg0==-1 || msg->arg0>=ndev || fcb[dev[msg->arg0].fid].device.blksize<=1) continue;
            // if we have cleared the last dirty block and we have a nomem situation, free cache
            if(cache_cleardirty(msg->arg0, msg->arg1) && nomem) {
                cache_free();
                nomem=false;
            }
            continue;
        } else {
            // get task context or if not found, create it on demand
            ctx = taskctx_get(EVT_SENDER(msg->evt));
            // new query (not a resumed one), reset work counter
            ctx->workleft = -1;
        }
        /* serve requests */
        switch(EVT_FUNC(msg->evt)) {
#if DEBUG
            case SYS_fsdump:
                taskctx_dump();
                fsdrv_dump();
                devfs_dump();
                mtab_dump();
                fcb_dump();
                cache_dump();
                break;
#endif

            // task is exiting, clean up
            case SYS_exit:
                for(j=0;j<ctx->nopenfiles;j++)
                    taskctx_close(ctx, j, false);
                for(j=0;j<ndev;j++)
                    if(dev[j].drivertask==ctx->pid)
                        devfs_del(j);
                fcb_cleanup();
                continue;

            case SYS_mountfs:
#if DEBUG
                if(_debug&DBG_FS) {
                    dbg_printf("FS: mountfs()\n");
                }
#endif
                mtab_fstab(_fstab_ptr, _fstab_size);
#if DEBUG
                if(_debug&DBG_FS)
                    mtab_dump();
#endif
                // notify other system services that file system is ready,
                // they can continue with their initialization
                mq_send(SRV_UI, SYS_ack, 0);
                mq_send(SRV_syslog, SYS_ack, 0);
                mq_send(SRV_inet, SYS_ack, 0);
                mq_send(SRV_sound, SYS_ack, 0);
                mq_send(SRV_init, SYS_ack, 0);
                continue;

            case SYS_mount:
                if(msg->ptr==NULL || msg->size==0) {
                    seterr(EINVAL);
                    ret=-1;
                    break;
                }
                j=strlen(msg->ptr);
                k=strlen(msg->ptr + j+1);
#if DEBUG
                if(_debug&DBG_FS)
                    dbg_printf("FS: mount(%s, %s, %s)\n",msg->ptr, msg->ptr+j+1, msg->ptr+j+1+k+1);
#endif
                ret = mtab_add(msg->ptr, msg->ptr+j+1, msg->ptr+j+1+k+1)!=-1 ? 0 : -1;
#if DEBUG
                if(_debug&DBG_FS)
                    mtab_dump();
#endif
                break;

            case SYS_umount:
#if DEBUG
                if(_debug&DBG_FS)
                    dbg_printf("FS: umount(%s)\n",msg->ptr);
#endif
                if(msg->ptr==NULL || msg->size==0) {
                    seterr(EINVAL);
                    ret=-1;
                    break;
                }
                ret = mtab_del(msg->ptr, msg->ptr) ? 0 : -1;
                fcb_cleanup();
#if DEBUG
                if(_debug&DBG_FS)
                    mtab_dump();
#endif
                break;

            case SYS_fsck:
#if DEBUG
                if(_debug&DBG_DEVICES)
                    dbg_printf("FS: fsck(%s)\n",msg->ptr);
#endif
                ret=lookup(msg->ptr, false);
                if(fsck.dev!=-1 && fsck.dev!=ret) {
                    seterr(EBUSY);
                    ret=-1;
                    break;
                }
                if(ret!=-1 && !errno()) {
                    ret=dofsck(ret, msg->type) ? 0 : -1;
                    if(!ret)
                        break;
                }
                seterr(ENODEV);
                ret=-1;
                break;

            case SYS_mknod:
#if DEBUG
                if(_debug&DBG_DEVICES)
                    dbg_printf("FS: mknod(%s,%d,%02x,%d,%d)\n",msg->ptr,msg->type,msg->attr0,msg->attr1,msg->attr2);
#endif
                if(msg->ptr==NULL || msg->size==0) {
                    seterr(EINVAL);
                    ret=-1;
                    break;
                }
                ret = devfs_add(msg->ptr, ctx->pid, msg->type, msg->attr0, msg->attr1, msg->attr2) == -1 ? -1 : 0;
//devfs_dump();
                break;

            case SYS_ioctl:
#if DEBUG
                if(_debug&DBG_DEVICES)
                    dbg_printf("FS: ioctl(%d,%d,%s[%d])\n",msg->type,msg->attr0,msg->ptr,msg->size);
#endif
                if(!taskctx_validfid(ctx,msg->type) || fcb[ctx->openfiles[msg->type].fid].type!=FCB_TYPE_DEVICE) {
                    seterr(EBADF);
                    ret=-1;
                } else {
                    // relay the message to the appropriate driver task with device id
                    devfs_t *df=&dev[fcb[ctx->openfiles[msg->type].fid].device.inode];
#if DEBUG
                if(_debug&DBG_DEVICES)
                    dbg_printf("FS: ioctl from %x to pid %x dev %d\n",ctx->pid, df->drivertask, df->device);
#endif
                    mq_send(df->drivertask, DRV_ioctl|(msg->ptr!=NULL&&msg->size>0?MSG_PTRDATA:0),
                        msg->ptr, msg->size, df->device, msg->attr0, ctx->pid);
                    continue;
                }
                break;

            case SYS_chroot:
                ret=lookup(msg->ptr, false);
                if(ret!=-1 && !errno()) {
                    if(fcb[ret].type!=FCB_TYPE_REG_DIR) {
                        seterr(ENOTDIR);
                        ret=-1;
                    } else {
                        fcb_del(ctx->rootdir);
                        fcb[ret].nopen++;
                        ctx->rootdir = ret;
                        // if workdir outside of new rootdir, use rootdir as workdir
                        if(memcmp(fcb[ctx->workdir].abspath, fcb[ctx->rootdir].abspath, strlen(fcb[ctx->rootdir].abspath))) {
                            fcb_del(ctx->workdir);
                            fcb[ret].nopen++;
                            ctx->workdir = ret;
                        }
                        ret = 0;
                    }
                }
                break;

            case SYS_chdir:
                ret=lookup(msg->ptr, false);
                if(ret!=-1 && !errno()) {
                    if(fcb[ret].type!=FCB_TYPE_REG_DIR) {
                        seterr(ENOTDIR);
                        ret=-1;
                    } else {
                        fcb_del(ctx->workdir);
                        // if new workdir outside of rootdir, use rootdir as workdir
                        if(memcmp(fcb[ret].abspath, fcb[ctx->rootdir].abspath, strlen(fcb[ctx->rootdir].abspath)))
                            ret=ctx->rootdir;
                        fcb[ret].nopen++;
                        ctx->workdir = ret;
                        ret = 0;
                    }
                }
                break;

            case SYS_getcwd:
                ptr=fcb[ctx->workdir].abspath;
                ret=strlen(fcb[ctx->rootdir].abspath);
                if(!memcmp(ptr, fcb[ctx->rootdir].abspath, ret))
                    ptr+=ret-1;
                else
                    ptr="/";
                mq_send(EVT_SENDER(msg->evt), SYS_ack | MSG_PTRDATA, ptr, strlen(ptr)+1);
                ctx->msg.evt = 0;
                continue;

            case SYS_realpath:
                ret=lookup(msg->ptr, false);
                if(ret!=-1 && !errno()) {
                    ptr=fcb[ret].abspath;
                    mq_send(EVT_SENDER(msg->evt), SYS_ack | MSG_PTRDATA, ptr, strlen(ptr)+1);
                    ctx->msg.evt = 0;
                    continue;
                }
                ret=0;
                break;

            case SYS_dstat:
                if(msg->arg0>=0 && msg->arg0<nfcb && fcb[msg->arg0].type==FCB_TYPE_DEVICE) {
                    ret=msg->arg0;
                    goto dostat;
                }
                seterr(EINVAL);
                ret=0;
                break;

            case SYS_fstat:
                if(taskctx_validfid(ctx,msg->arg0)) {
                    ret=ctx->openfiles[msg->arg0].fid;
                    goto dostat;
                }
                seterr(EBADF);
                ret=0;
                break;

            case SYS_lstat:
                ret=lookup(msg->ptr, false);
dostat:         if(ret!=-1 && !errno()) {
                    ptr=statfs(ret);
                    if(ptr!=NULL)
                        p2pcpy(EVT_SENDER(msg->evt), (void*)msg->arg2, ptr, sizeof(stat_t));
                    else
                        seterr(ENOENT);
                }
                ret=0;
                break;

            case SYS_tmpfile:
//dbg_printf("fs tmpfile()\n");
                ptr=malloc(128);
                if(ptr==NULL) {
                    ret=-1;
                    break;
                }
                //FIXME: no sprintf yet
                //sprintf(ptr, "%s%x%x", P_tmpdir, ctx->pid, rand());
                ret=lookup(ptr, true);
                if(ret!=-1 && !errno()) {
                    ret=taskctx_open(ctx,ret,O_RDWR|O_TMPFILE,0,-1);
                }
                free(ptr);
//taskctx_dump();
                break;

            case SYS_fopen:
//dbg_printf("fs fopen(%s, %x)\n", msg->ptr, msg->type);
                ret=lookup(msg->ptr, msg->type & O_CREAT ? true : false);
                if(ret!=-1 && !errno()) {
                    if(fcb[ret].type==FCB_TYPE_REG_DIR || fcb[ret].type==FCB_TYPE_UNION) {
                        seterr(EISDIR);
                        ret=-1;
                    } else {
                        if(msg->attr0!=-1)
                            taskctx_close(ctx, msg->attr0, true);
                        // get offset part of path
                        o=getoffs(ptr);
                        if(o<0) o+=fcb[ret].reg.filesize;
                        // append flag overwrites it
                        if(msg->type & O_APPEND) o=fcb[ret].reg.filesize;
                        ret=taskctx_open(ctx,ret,msg->type,o,msg->attr0);
                    }
                }
//fcb_dump();
//taskctx_dump();
                break;

          //case SYS_closedir:
            case SYS_fclose:
//dbg_printf("fs %s %d\n",ctx->openfiles[msg->arg0].mode&OF_MODE_READDIR?"closedir":"fclose",msg->arg0);
                if(!taskctx_validfid(ctx,msg->arg0)) {
                    seterr(EBADF);
                    ret=-1;
                } else {
                    if(ctx->openfiles[msg->arg0].mode & O_WRITE)
                        cache_flush(ctx->openfiles[msg->arg0].fid);
                    ret=taskctx_close(ctx, msg->arg0, false) ? 0 : -1;
                }
//taskctx_dump();
                break;

            case SYS_fcloseall:
//dbg_printf("fs fcloseall\n");
                cache_flush(0);
                for(j=0;j<ctx->nopenfiles;j++)
                    taskctx_close(ctx, j, false);
                fcb_cleanup();
                ret=0;
//taskctx_dump();
                break;

            case SYS_fseek:
                ret=taskctx_seek(ctx, msg->arg0, msg->arg1, msg->arg2) ? 0 : -1;
                if(ret && !errno())
                    seterr(EINVAL);
                break;

            case SYS_rewind:
                // don't use taskctx_seek here, as it only allows regular files, and we
                // want rewind() to work on directory streams too
                if(!taskctx_validfid(ctx,msg->arg0)) {
                    seterr(EBADF);
                    ret=-1;
                } else {
                    ctx->openfiles[msg->arg0].offs = 0;
                    ctx->openfiles[msg->arg0].unionidx = 0;
                    ret=0;
                }
                break;

            case SYS_ftell:
                if(!taskctx_validfid(ctx,msg->arg0)) {
                    seterr(EBADF);
                    ret=0;
                } else
                    ret=ctx->openfiles[msg->arg0].offs;
                break;

            case SYS_feof:
                if(!taskctx_validfid(ctx,msg->arg0)) {
                    seterr(EBADF);
                    ret=-1;
                } else
                    ret=fcb[ctx->openfiles[msg->arg0].fid].abspath==NULL || 
                        ctx->openfiles[msg->arg0].mode&OF_MODE_EOF || 
                        ctx->openfiles[msg->arg0].offs >= fcb[ctx->openfiles[msg->arg0].fid].reg.filesize;
                break;

            case SYS_ferror:
                if(!taskctx_validfid(ctx,msg->arg0)) {
                    seterr(EBADF);
                    ret=-1;
                } else
                    // force EOF flag on closed files
                    ret=ctx->openfiles[msg->arg0].mode >> 16 | (fcb[ctx->openfiles[msg->arg0].fid].abspath==NULL?1:0);
                break;

            case SYS_fclrerr:
                if(!taskctx_validfid(ctx,msg->arg0)) {
                    seterr(EBADF);
                    ret=-1;
                } else {
                    ctx->openfiles[msg->arg0].mode &= 0xffff;
                    ret=0;
                }
                break;

            case SYS_dup:
//dbg_printf("fs dup(%d)\n",msg->arg0);
                if(!taskctx_validfid(ctx,msg->arg0)) {
                    seterr(EBADF);
                    ret=-1;
                } else {
                    ret=taskctx_open(ctx, ctx->openfiles[msg->arg0].fid, ctx->openfiles[msg->arg0].mode,
                        ctx->openfiles[msg->arg0].offs,-1);
//taskctx_dump();
                }
                break;

            case SYS_dup2:
//dbg_printf("fs dup2(%d,%d)\n",msg->arg0,msg->arg1);
                if(!taskctx_validfid(ctx,msg->arg0)) {
                    seterr(EBADF);
                    ret=-1;
                } else {
                    // don't use task_validfid here, we only check if the descriptor is not unused
                    if(ctx->nopenfiles>msg->arg1 && ctx->openfiles[msg->arg1].fid!=-1) {
                        // don't free openfiles block, just mark it unused
                        taskctx_close(ctx, msg->arg1, true);
                    }
                    ret=taskctx_open(ctx, ctx->openfiles[msg->arg0].fid, ctx->openfiles[msg->arg0].mode,
                        ctx->openfiles[msg->arg0].offs, msg->arg1);
//taskctx_dump();
                }
                break;

            case SYS_fread:
//dbg_printf("fs fread(%d,%x,%d) offs %d mode %x\n",msg->type,msg->ptr,msg->size,
//    ctx->openfiles[msg->type].offs,ctx->openfiles[msg->type].mode);
                // buffers bigger than 1M must use shared memory
                if(msg->size>=__BUFFSIZE && (int64_t)msg->ptr > 0) {
                    seterr(ENOTSHM);
                    ret=0;
                } else
                if(msg->ptr==NULL || msg->size==0) {
                    // when buffer size 0, nothing to read
                    // when no buffer, report error
                    if(msg->ptr==NULL && msg->size>0)
                        seterr(EINVAL);
                    ret=0;
                } else
                    ret=taskctx_read(ctx,msg->type,(virt_t)msg->ptr,msg->size);
                break;
            
            case SYS_fwrite:
//dbg_printf("fs fwrite(%d,%x,%d)\n",msg->type,msg->ptr,msg->size);
                if(!taskctx_validfid(ctx,msg->type) ||
                   fcb[ctx->openfiles[msg->type].fid].type==FCB_TYPE_REG_DIR ||
                   fcb[ctx->openfiles[msg->type].fid].type==FCB_TYPE_UNION) {
                    seterr(EBADF);
                    ret=0;
                } else
                if(msg->ptr==NULL || msg->size==0) {
                    // nothing to write
                    // when no buffer, report error
                    if(msg->ptr==NULL && msg->size>0)
                        seterr(EINVAL);
                    ret=0;
                } else
                    ret=taskctx_write(ctx,msg->type,msg->ptr,msg->size);
                break;

            case SYS_opendir:
                if(msg->type==-1) {
//dbg_printf("fs opendir(%s,%d)\n", msg->ptr,msg->type);
                    ret=lookup(msg->ptr, false);
                } else {
//dbg_printf("fs opendir(%x[%d],%d)\n", msg->ptr,msg->size,msg->type);
                    if(!taskctx_validfid(ctx,msg->type)) {
                        seterr(EBADF);
                        ret=-1;
                        break;
                    } else
                    if(!(ctx->openfiles[msg->type].mode & O_READ) ||
                       !(fcb[ctx->openfiles[msg->type].fid].mode & O_READ)) {
                            seterr(EACCES);
                            ret=-1;
                            break;
                    }
                    ret=ctx->openfiles[msg->type].unionidx;
                    ctx->openfiles[msg->type].unionidx=0;
                }
                if(ret!=-1 && !errno()) {
                    if(fcb[ret].type!=FCB_TYPE_REG_DIR && fcb[ret].type!=FCB_TYPE_UNION) {
                        seterr(ENOTDIR);
                        ret=-1;
                    } else {
                        j=-1;
                        if(fcb[ret].type==FCB_TYPE_UNION) {
                            j=fcb_unionlist_build(ret, (void*)(msg->type==-1?NULL:msg->ptr), msg->size);
                            if(ackdelayed) break;
                            if(j!=-1) {
                                k=ret;
                                ret=taskctx_open(ctx,j,O_RDONLY,0,msg->type);
                                if(ret==-1)
                                    break;
                                ctx->openfiles[ret].unionidx=k;
                                ptr=statfs(j);
                                if(ptr==NULL || ((stat_t*)ptr)->st_size==0)
                                    taskctx_close(ctx, ret, true);
                                else {
                                    mq_send(EVT_SENDER(msg->evt), SYS_ack, ret, EVT_FUNC(msg->evt), 
                                        msg->serial, ((stat_t*)ptr)->st_size);
                                    ctx->msg.evt = 0;
                                    continue;
                                }
                            }
                        }
                        if(msg->type!=-1) {
                            taskctx_close(ctx, msg->type, true);
//dbg_printf("opening an union at %d\n",ret);fcb_dump();
                        }
                        ret=taskctx_open(ctx,ret,O_RDONLY,0,msg->type);
                        if(ret!=-1) {
                            ctx->openfiles[ret].mode|=OF_MODE_READDIR;
                        }
                    }
                }
//taskctx_dump();
                break;

            case SYS_readdir:
//dbg_printf("fs readdir(%x,%d,%d,%x)\n", msg->ptr, msg->size, msg->type,msg->attr0);
                if(!taskctx_validfid(ctx,msg->type) || (fcb[ctx->openfiles[msg->type].fid].type!=FCB_TYPE_REG_DIR &&
                  fcb[ctx->openfiles[msg->type].fid].type!=FCB_TYPE_UNION)) {
                    seterr(EBADF);
                    ret=0;
                } else {
                    ptr=(void*)readdirfs(ctx,msg->type,msg->ptr,msg->size);
                    if(ptr!=NULL && !errno()) {
                        p2pcpy(EVT_SENDER(msg->evt), (void*)msg->attr0, ptr, sizeof(dirent_t));
                        ret=0;
                    }
                }
                break;

            default:
#if DEBUG
                dbg_printf("FS: unknown event: %d from pid %x\n",EVT_FUNC(msg->evt), EVT_SENDER(msg->evt));
#endif
                seterr(EPERM);
                ret=-1;
                break;
        }
        // try to be ahead of out of ram scenario
        m=meminfo();j=m.freepages*100/m.totalpages;
        if(j<cachelimit || errno()==ENOMEM) {
            fcb_cleanup();
            // when out of memory, flush the cache, and clear nomem only when finished (and cache freed)
            cache_flush();
            nomem=true;
        }
        if(!ackdelayed) {
            // send result back to caller
            j = errno();
            mq_send(EVT_SENDER(msg->evt), j?SYS_nack:SYS_ack, j?j/*errno*/:ret/*return value*/,
                EVT_FUNC(msg->evt), msg->serial, false);
            // clear delayed message buffer
            ctx->msg.evt = 0;
        } else {
            // save message for later
            memcpy(&ctx->msg, msg, sizeof(msg_t));
        }
    }
    /* should never reach this */
}
