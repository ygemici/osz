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
#include "vfs.h"

public uint8_t *_initrd_ptr=NULL;   // initrd offset in memory
public uint64_t _initrd_size=0;     // initrd size in bytes
public char *_fstab_ptr=NULL;       // pointer to /etc/fstab data
public size_t _fstab_size=0;        // fstab size
public msg_t *msg;
uint8_t ackdelayed;
extern uint32_t _pathmax;           // maximum length of path

/* should be in cache.h and devfs.h, but nobody else allowed to call them */
extern void cache_init();
extern void devfs_init();
extern void devfs_dump();

uint64_t delayanswer()
{
    /* find the first slot in delayedmsg and copy msg, increase numblk, return index */
    ackdelayed=true;
    return 0;
}

void _init(int argc, char **argv)
{
    fid_t fid;
    if(_pathmax<512)
        _pathmax=512;
    /* allocate directory and block caches */
    cache_init();
    /* initialize dev directory, and in memory "block devices" */
    devfs_init();
    /*** endless loop ***/
    while(1) {
        /* get work */
        msg = mq_recv();
        ackdelayed = false;
        /* resume a posponed query */
        if(EVT_FUNC(msg->evt)==SYS_setblock) {
            /* TODO: check if caller is a driver task, PRI_DRV and msg->argX valid delayedmsg index */
            cache_setblock(msg);
            /* TODO: find delayedmsg, decrease numblk */
            //delayedmsg[msg->arg3].numblk--;
            //if(!delayedmsg[msg->arg3].numblk)
            //  memcpy(msg,&delayedmsg[msg->arg3],sizeof(msg_t));
            //else
            //  continue;
        }
        /* serve requests */
        switch(EVT_FUNC(msg->evt)) {
            case SYS_mountfs:
devfs_dump();
//cache_dump();
                vfs_fstab(_fstab_ptr, _fstab_size);
vfs_dump();
                break;

            case SYS_locate:
                // save path start pointer
                msg->arg3 = msg->arg1;
                fid=vfs_locate((fid_t)msg->arg0, (char*)msg->arg1, (uint64_t)msg->arg2);
                if(!ackdelayed) {
                    mq_send(EVT_SENDER(msg->evt), errno == SUCCESS ? SYS_ack : SYS_nack,
                        fid/*return value*/, errno, SYS_locate, msg->serial);
                }
                break;

            default:
#if DEBUG
                dbg_printf("FS: unknown event: %x\n",EVT_FUNC(msg->evt));
#endif
                break;
        }
    }
    /* should never reach this */
}
