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
#include "cache.h"
#include "mtab.h"
#include "devfs.h"
#include "fsdrv.h"
#include "taskctx.h"
#include "fcb.h"
#include "vfs.h"

//#include "vfs.h"

public uint8_t *_initrd_ptr=NULL;   // initrd offset in memory
public uint64_t _initrd_size=0;     // initrd size in bytes
public char *_fstab_ptr=NULL;       // pointer to /etc/fstab data
public size_t _fstab_size=0;        // fstab size
public uint32_t _pathmax;           // maximum length of path
uint8_t ackdelayed;

/**
 * File System task main procedure
 */
void _init(int argc, char **argv)
{
    msg_t *msg;
    uint64_t ret = 0, j;
    off_t o;
    char *ptr;

    // failsafes
    if(_pathmax<512)
        _pathmax=512;
    // initialize vfs structures
    mtab_init();    // registers "/" as first fcb
    devfs_init();   // registers "/dev/" as second fcb
    cache_init();

    /*** endless loop ***/
    while(1) {
        /* get work */
        msg = mq_recv();
        seterr(SUCCESS);
        ctx = taskctx_get(EVT_SENDER(msg->evt));
        ackdelayed = false;
        // if it's a block ready message
        if(EVT_FUNC(msg->evt) == SYS_setblock) {
            // save it to block cache
            cache_setblock((void*)msg->arg0/*blk*/, msg->arg1/*size*/, msg->arg2/*dev*/, msg->arg3/*offs*/);
            // resume original message that caused the block read
            ctx = taskctx_get(msg->arg4);
            msg = &ctx->msg;
        }
        /* serve requests */
        switch(EVT_FUNC(msg->evt)) {
            case SYS_mountfs:
                mtab_fstab(_fstab_ptr, _fstab_size);
                break;

            case SYS_chroot:
                ret=lookup(msg->ptr);
                if(ret!=-1 && !errno()) {
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
                break;

            case SYS_chdir:
                ret=lookup(msg->ptr);
                if(ret!=-1 && !errno()) {
                    fcb_del(ctx->workdir);
                    // if new workdir outside of rootdir, use rootdir as workdir
                    if(memcmp(fcb[ret].abspath, fcb[ctx->rootdir].abspath, strlen(fcb[ctx->rootdir].abspath)))
                        ret=ctx->rootdir;
                    fcb[ret].nopen++;
                    ctx->workdir = ret;
                    ret = 0;
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

            case SYS_fopen:
dbg_printf("fs fopen(%s, %x)\n", msg->ptr, msg->attr0);
                ptr=canonize(msg->ptr);
                if(ptr==NULL)
                    break;
                ret=lookup(msg->ptr);
                if(ret!=-1 && !errno()) {
                    o=getoffs(ptr);
                    if(o<0) o+=fcb[ret].reg.filesize;
                    if(msg->attr0&O_APPEND) o=fcb[ret].reg.filesize;
                    ret=taskctx_open(ctx,ret,msg->attr0,o);
                }
                free(ptr);
taskctx_dump();
                break;

            case SYS_fclose:
                if(taskctx_close(ctx, msg->arg0))
                    ret=0;
                else
                    ret=-1;
                break;

            case SYS_fseek:
                if(taskctx_seek(ctx, msg->arg0, msg->arg1, msg->arg2))
                    ret=0;
                else
                    ret=-1;
                break;

            case SYS_rewind:
                if(ctx->openfiles==NULL || ctx->nopenfiles<=msg->arg0 || ctx->openfiles[msg->arg0].fid==-1) {
                    seterr(EINVAL);
                    ret=-1;
                } else {
                    ctx->openfiles[msg->arg0].offs = 0;
                    ret=0;
                }
                break;

            case SYS_ftell:
                if(ctx->openfiles==NULL || ctx->nopenfiles<=msg->arg0 || ctx->openfiles[msg->arg0].fid==-1) {
                    seterr(EINVAL);
                    ret=0;
                } else
                    ret=ctx->openfiles[msg->arg0].offs;
                break;

            case SYS_feof:
                if(ctx->openfiles==NULL || ctx->nopenfiles<=msg->arg0 || ctx->openfiles[msg->arg0].fid==-1) {
                    seterr(EINVAL);
                    ret=1;
                } else
                    ret=ctx->openfiles[msg->arg0].mode&0x10000 || 
                        ctx->openfiles[msg->arg0].offs >= fcb[ctx->openfiles[msg->arg0].fid].reg.filesize;
                break;

            case SYS_ferror:
                if(ctx->openfiles==NULL || ctx->nopenfiles<=msg->arg0 || ctx->openfiles[msg->arg0].fid==-1) {
                    seterr(EINVAL);
                    ret=1;
                } else
                    ret=ctx->openfiles[msg->arg0].mode >> 16;
                break;

            case SYS_fclrerr:
                if(ctx->openfiles==NULL || ctx->nopenfiles<=msg->arg0 || ctx->openfiles[msg->arg0].fid==-1) {
                    seterr(EINVAL);
                } else
                    ctx->openfiles[msg->arg0].mode &= 0xffff;
                break;

            default:
#if DEBUG
                dbg_printf("FS: unknown event: %x\n",EVT_FUNC(msg->evt));
#endif
                seterr(EPERM);
                break;
        }
        if(!ackdelayed) {
            // send result back to caller
            j = errno();
            mq_send(EVT_SENDER(msg->evt), j?SYS_nack:SYS_ack, j?j/*errno*/:ret/*return value*/,
                EVT_FUNC(msg->evt), msg->serial);
            // clear delayed message buffer
            ctx->msg.evt = 0;
        } else {
            // save message for later
            memcpy(&ctx->msg, msg, sizeof(msg_t));
        }
    }
    /* should never reach this */
}
