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
#include "taskctx.h"
#include "fcb.h"

//#include "vfs.h"

public uint8_t *_initrd_ptr=NULL;   // initrd offset in memory
public uint64_t _initrd_size=0;     // initrd size in bytes
public char *_fstab_ptr=NULL;       // pointer to /etc/fstab data
public size_t _fstab_size=0;        // fstab size
uint8_t ackdelayed;
extern uint32_t _pathmax;           // maximum length of path

void _init(int argc, char **argv)
{
    msg_t *msg;
    uint64_t ret = 0;
    // failsafes
    if(_pathmax<512)
        _pathmax=512;
    // initialize vfs structures
    mtab_init();    // registers "/" as first fcb
    cache_init();
    devfs_init();

    /*** endless loop ***/
    while(1) {
        /* get work */
        msg = mq_recv();
        seterr(SUCCESS);
dbg_printf("%x: %x\n",msg,msg->evt);
        ctx = taskctx_get(EVT_SENDER(msg->evt));
        ackdelayed = false;
        /* resume a posponed query */
        if(EVT_FUNC(msg->evt) == SYS_setblock) {
            cache_setblock(msg);
            ctx = taskctx_get(msg->arg3);
            msg = &ctx->msg;
        }
        /* serve requests */
        switch(EVT_FUNC(msg->evt)) {
            case SYS_mountfs:
dbg_printf("mountfs\n");
devfs_dump();
//cache_dump();
                mtab_fstab(_fstab_ptr, _fstab_size);
fcb_dump();
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
            mq_send(EVT_SENDER(msg->evt), errno() == SUCCESS ? SYS_ack : SYS_nack,
                ret, errno(), EVT_FUNC(msg->evt), msg->serial);
            // clear delayed message buffer
            ctx->msg.evt = 0;
        } else {
            // save delayed message for later
            memcpy(&ctx->msg, msg, sizeof(msg_t));
        }
    }
    /* should never reach this */
}
