/*
 * fs/cache.c
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
 * @brief Cache implementation
 */
#include <osZ.h>
#include <sys/driver.h>
#include "taskctx.h"
#include "cache.h"
#include "fcb.h"
#include "devfs.h"

extern uint8_t ackdelayed;      // flag to indicate async block read
extern uint16_t cachelines;     // number of cache lines
fid_t cache_currfid=0;          // fcb id of current open file
uint64_t cache_inflush=0;       // number of blocks in flush

typedef struct {
    fid_t fd;
    blkcnt_t lsn;
    fid_t fid;
    void *blk;
} cache_t;

typedef struct {
    uint64_t ncache;
    cache_t *blks;
} cacheline_t;

cacheline_t *cache;

void cache_init()
{
    // failsafe
    if(cachelines<16)
        cachelines=16;
    cache=(cacheline_t*)malloc(cachelines*sizeof(cacheline_t));
    // abort if we cannot allocate cache
    if(cache==NULL || errno())
        exit(2);
}

/**
 * read a block from cache, sets ackdelayed on cache miss
 */
public void* cache_getblock(fid_t fd, blkcnt_t lsn)
{
    uint64_t i,j=lsn%cachelines;
#if DEBUG
    if(_debug&DBG_CACHE)
        dbg_printf("FS: cache_getblock(fd %d, sector %d)\n",fd,lsn);
#endif
    if(fd==(fid_t)-1 || fd>=nfcb || lsn==(blkcnt_t)-1 || fcb[fd].type!=FCB_TYPE_DEVICE || cache[j].ncache==0)
        return NULL;
    for(i=0;i<cache[j].ncache;i++)
        if(cache[j].blks[i].fd==fd && cache[j].blks[i].lsn==lsn)
            return cache[j].blks[i].blk;
    j=fcb[fd].device.inode;
    // block not found in cache. Send a message to a driver
    mq_send(dev[j].drivertask, DRV_read, dev[j].device, lsn, fcb[fd].device.blksize, ctx->pid, j);
    // delay ack message to the original caller
    ackdelayed = true;
    return NULL;
}

/**
 * store a block in cache, called by drivers
 */
public bool_t cache_setblock(fid_t fd, blkcnt_t lsn, void *blk, blkprio_t prio)
{
    uint64_t i,j=lsn%cachelines,bs,f=-1;
    void *ptr;
    cache_t *cptr;
#if DEBUG
    if(_debug&DBG_CACHE)
        dbg_printf("FS: cache_setblock(fd %d, sector %d, buf %x, prio %d) fid %d\n",fd,lsn,blk,prio,cache_currfid);
#endif
    if(fd==-1 || fd>=nfcb || lsn==-1 || fcb[fd].type!=FCB_TYPE_DEVICE)
        return false;
    bs=fcb[fd].device.blksize;
    if(bs<=1)
        return false;
    for(i=0;i<cache[j].ncache;i++) {
        cptr=&cache[j].blks[i];
        if(cptr->fd==fd && cptr->lsn==lsn) {
            cptr->fid=cache_currfid|((uint64_t)prio<<60);
            memcpy(cptr->blk, blk, bs);
            return true;
        }
        if(f==-1 && cptr->fid==-1)
            f=i;
    }
    if(f==-1) {
        f=cache[j].ncache;
        cache[j].blks=(cache_t*)realloc(cache[j].blks,++cache[j].ncache*sizeof(cache_t));
        if(cache[j].blks==NULL)
            return false;
    }
    ptr=malloc(bs);
    if(ptr==NULL)
        return false;
    memcpy(ptr,blk,bs);
    cptr=&cache[j].blks[f];
    cptr->fd=fd;
    cptr->lsn=lsn;
    cptr->fid=cache_currfid|((uint64_t)prio<<60);
    cptr->blk=ptr;
    return true;
}

/**
 * remove all blocks of a device from cache
 */
public blkcnt_t cache_freeblocks(fid_t fd, blkcnt_t needed)
{
    uint64_t i,j;
    blkcnt_t cnt=0;
    cache_t *cptr;
#if DEBUG
    if(_debug&DBG_CACHE)
        dbg_printf("FS: cache_freeblocks(fd %d, needed %d)\n",fd,needed);
#endif
    for(j=0;j<cachelines && (needed==0 || cnt<needed);j++) {
        for(i=cache[j].ncache-1;i>=0 && (needed==0 || cnt<needed);i--) {
            cptr=&cache[j].blks[i];
            if(cptr->fd==fd) {
                free(cptr->blk);
                cptr->fd=-1;
                cnt++;
            }
        }
        i=cache[j].ncache; while(i>0 && cache[j].blks[i].fd==-1) i--;
        if(i!=cache[j].ncache) {
            cache[j].ncache=i;
            cache[j].blks=(cache_t*)realloc(cache[j].blks,cache[j].ncache*sizeof(cache_t));
        }
    }
    return cnt;
}

/**
 * flush cache to device
 */
void cache_flush(fid_t fid)
{
    uint64_t i,j,k,p,pm;
    fid_t f;
    cache_t *cptr;
    for(p=BLKPRIO_CRIT;p<=BLKPRIO_META;p++) {
        pm=(p<<60);
        for(j=0;j<cachelines;j++) {
            for(i=0;i<cache[j].ncache;i++) {
                cptr=&cache[j].blks[i];
                f=cptr->fid & 0xFFFFFFFFFFFFFFFUL;
                if((cptr->fid & pm) && (f==0 || fid==-1 || f==fid)) {
                    k=fcb[cptr->fd].device.inode;
                    // block dirty in cache. Send a message to a driver to write out
                    mq_send(dev[k].drivertask, DRV_write|MSG_PTRDATA, cptr->blk, fcb[cptr->fd].device.blksize,
                        dev[k].device, cptr->lsn, k, cptr, ctx->pid);
                    dev[k].total++;
                    cache_inflush++;
                }
            }
        }
    }
#if DEBUG
    if(_debug&DBG_CACHE)
        dbg_printf("FS: cache_flush(fid %d) flushing %d blks\n",fid,cache_inflush);
#endif
}

/**
 * clear dirty flag on cache block and increase written out counter on dev
 */
bool_t cache_cleardirty(uint64_t idx,void *cptr)
{
    // increase number of blocks written out
    dev[idx].out++;
    if(dev[idx].out >= dev[idx].total)
        dev[idx].out=dev[idx].total=0;
    // if arg1 is a cache block, clear dirty flag
    if(cptr!=NULL && ((cache_t*)cptr)->fd==dev[idx].fid)
        ((cache_t*)cptr)->fid &= 0xFFFFFFFFFFFFFFFUL;
    // notify UI to update progressbar on dock icons
    mq_send(SRV_UI, SYS_devprogress, dev[idx].fid, dev[idx].out, dev[idx].total);
    cache_inflush--;
#if DEBUG
    if(_debug&DBG_CACHE && cache_inflush==0)
        dbg_printf("FS: cache_cleardirty() all blks flushed\n");
#endif
    return cache_inflush==0;
}

/**
 * free up at least half of the cache by releasing last recently used blocks
 */
void cache_free()
{
    uint64_t i,j,k;
    blkcnt_t total=0,freed=0;
    for(j=0;j<cachelines;j++)
        for(i=0;i<cache[j].ncache;i++)
            if(cache[j].blks[i].fd!=-1)
                total++;
#if DEBUG
    if(_debug&DBG_CACHE)
        dbg_printf("FS: cache_free() total %d blks\n", total);
#endif
    total/=2; i=0;k=-1;
    // failsafe, got the same device as last time?
    while(freed<total && i!=k) {
        i=k;
        // get the last recently used device
        k=devfs_lastused(false);
        // free all of it's blocks in cache
        freed+=cache_freeblocks(dev[k].fid, total-freed);
    }
#if DEBUG
    if(_debug&DBG_CACHE)
        dbg_printf("FS: cache_free() freed %d blks\n", freed);
#endif
}

#if DEBUG
void cache_dump()
{
}
#endif
