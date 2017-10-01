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
#include "fsdrv.h"
#include "taskctx.h"
#include "fcb.h"
#include "vfs.h"

//#include "vfs.h"

public uint8_t *_initrd_ptr=NULL;   // initrd offset in memory
public uint64_t _initrd_size=0;     // initrd size in bytes
public char *_fstab_ptr=NULL;       // pointer to /etc/fstab data
public size_t _fstab_size=0;        // fstab size
public uint32_t pathmax;            // maximum length of path
uint8_t ackdelayed;

/**
 * File System task main procedure
 */
void _init(int argc, char **argv)
{
    msg_t *msg;
    uint64_t ret = 0, j, k;
    off_t o;
    char *ptr;

    // environment
    pathmax=env_num("pathmax",512,256,1024*1024);

    // initialize vfs structures
    mtab_init();    // registers "/" as first fcb
    devfs_init();   // registers "/dev/" as second fcb
    cache_init();

    /*** endless loop ***/
    while(1) {
        /* get work */
        msg = mq_recv();
        // clear state
        seterr(SUCCESS);
        ackdelayed = false;
        ret=0;
        // if it's a block ready message
        if(EVT_FUNC(msg->evt) == SYS_setblock) {
            // save it to block cache
            cache_setblock(msg->ptr/*blk*/, msg->size, msg->attr0/*dev*/, msg->attr1/*offs*/);
            // resume original message that caused it. Don't use taskctx_get here,
            // as that would always return a valid context
            ctx=taskctx[msg->arg4 & 0xFF]; if(ctx==NULL) continue;
            while(ctx!=NULL) { if(ctx->pid==msg->arg4) break; ctx=ctx->next; }
            if(ctx==NULL || EVT_FUNC(ctx->msg.evt)==0) continue;
            // ok, msg->arg4 points to a pid with a valid context and with a suspended message
            msg = &ctx->msg;
        } else {
            // get task context and create it on demand
            ctx = taskctx_get(EVT_SENDER(msg->evt));
        }
        /* serve requests */
        switch(EVT_FUNC(msg->evt)) {
            case SYS_mountfs:
                mtab_fstab(_fstab_ptr, _fstab_size);
                break;

            case SYS_mount:
                j=strlen(msg->ptr);
                k=strlen(msg->ptr + j+1);
//dbg_printf("fs mount(%s, %s, %s)\n",msg->ptr, msg->ptr+j+1, msg->ptr+j+1+k+1);
                ret = mtab_add(msg->ptr, msg->ptr+j+1, msg->ptr+j+1+k+1)!=-1 ? 0 : -1;
//mtab_dump();
                break;

            case SYS_umount:
//dbg_printf("fs umount(%s)\n",msg->ptr);
                ret = mtab_del(msg->ptr, msg->ptr) ? 0 : -1;
//mtab_dump();
                break;

            case SYS_mknod:
//dbg_printf("fs mknod(%s,%d,%02x,%d,%d)\n",msg->ptr,msg->type,msg->attr0,msg->attr1,msg->attr2);
                ret = devfs_add(msg->ptr, ctx->pid, msg->type, msg->attr0, msg->attr1, msg->attr2) == -1 ? -1 : 0;
//devfs_dump();
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
//dbg_printf("fs fopen(%s, %x)\n", msg->ptr, msg->attr0);
                ptr=canonize(msg->ptr);
                if(ptr==NULL) {
                    ret=-1;
                    break;
                }
                ret=lookup(msg->ptr);
                if(ret!=-1 && !errno()) {
                    // get offset part of path
                    o=getoffs(ptr);
                    if(o<0) o+=fcb[ret].reg.filesize;
                    // append flag overwrites it
                    if(msg->attr0 & O_APPEND) o=fcb[ret].reg.filesize;
                    ret=taskctx_open(ctx,ret,msg->attr0,o);
                }
                free(ptr);
//taskctx_dump();
                break;

            case SYS_fclose:
                ret=taskctx_close(ctx, msg->arg0) ? 0 : -1;
//dbg_printf("fs fclose ret %d\n",ret);
//taskctx_dump();
                break;

            case SYS_fseek:
                ret=taskctx_seek(ctx, msg->arg0, msg->arg1, msg->arg2) ? 0 : -1;
                break;

            case SYS_rewind:
                // don't use taskctx_seek here, as it only allows regular files, and we want to use
                // rewind() on directories too for readdir()
                if(ctx->openfiles==NULL || ctx->nopenfiles<=msg->arg0 || ctx->openfiles[msg->arg0].fid==-1 ||
                    fcb[ctx->openfiles[msg->arg0].fid].abspath==NULL) {
                    seterr(EINVAL);
                    ret=-1;
                } else {
                    ctx->openfiles[msg->arg0].offs = 0;
                    ret=0;
                }
                break;

            case SYS_ftell:
                if(ctx->openfiles==NULL || ctx->nopenfiles<=msg->arg0 || ctx->openfiles[msg->arg0].fid==-1 ||
                    fcb[ctx->openfiles[msg->arg0].fid].abspath==NULL) {
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
                    ret=fcb[ctx->openfiles[msg->arg0].fid].abspath==NULL || 
                        ctx->openfiles[msg->arg0].mode&OF_MODE_EOF || 
                        ctx->openfiles[msg->arg0].offs >= fcb[ctx->openfiles[msg->arg0].fid].reg.filesize;
                break;

            case SYS_ferror:
                if(ctx->openfiles==NULL || ctx->nopenfiles<=msg->arg0 || ctx->openfiles[msg->arg0].fid==-1) {
                    seterr(EINVAL);
                    ret=1;
                } else
                    ret=ctx->openfiles[msg->arg0].mode >> 16 | (fcb[ctx->openfiles[msg->arg0].fid].abspath==NULL?1:0);
                break;

            case SYS_fclrerr:
                if(ctx->openfiles==NULL || ctx->nopenfiles<=msg->arg0 || ctx->openfiles[msg->arg0].fid==-1 ||
                    fcb[ctx->openfiles[msg->arg0].fid].abspath==NULL) {
                    seterr(EINVAL);
                } else
                    ctx->openfiles[msg->arg0].mode &= 0xffff;
                break;

            default:
#if DEBUG
                dbg_printf("FS: unknown event: %x\n",EVT_FUNC(msg->evt));
#endif
                seterr(EPERM);
                ret=-1;
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
