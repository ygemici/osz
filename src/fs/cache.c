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
uint64_t cache_inflush=0;       // number of blocks flushing

#define CACHE_FD(x) (x&0xFFFFFFFFFFFFFFFULL)
#define CACHE_PRIO(x) (x>>60)

// sizeof=32
typedef struct {
    fid_t fd;
    blkcnt_t lsn;
    void *blk;
    void *next;
} cache_t;

// sizeof=cachelines*8
cache_t **cache;

void cache_init()
{
    // failsafe
    if(cachelines<16)
        cachelines=16;
    cache=(cache_t**)malloc(cachelines*sizeof(cache_t*));
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
    cache_t *c,*l=NULL;
    blksize_t bs;
#if DEBUG
    if(_debug&DBG_CACHE)
        dbg_printf("FS: cache_getblock(fd %d, sector %d)\n",fd,lsn);
#endif
    // failsafe
    if(fd==-1 || fd>=nfcb || lsn==-1 || fcb[fd].type!=FCB_TYPE_DEVICE || cache[j]==NULL) return NULL;
    if((lsn+1)*fcb[fd].device.blksize>fcb[fd].device.filesize) { seterr(EFAULT); return NULL; }
    // walk through the cacheline chain looking for the block
    for(c=cache[j];c!=NULL;c=c->next) {
        if(CACHE_FD(c->fd)==fd && c->lsn==lsn) {
            if(l!=NULL)
                l->next=c->next;
            // move block in front of the chain
            c->next=cache[j];
            cache[j]=c;
            return c->blk;
        }
        l=c;
    }
    // block not found in cache. Send a message to the driver task to read it
    i=fcb[fd].device.inode;
    // we read more for first sector to allow proper detection of fs
    bs=fcb[fd].device.blksize;
    if(lsn==0 && bs<BUFSIZ)
        bs=BUFSIZ;
    mq_send(dev[i].drivertask, DRV_read, dev[i].device, lsn, bs, ctx->pid, i);
    // delay ack message to the original caller (will be resumed when driver fills in cache)
    ackdelayed = true;
    return NULL;
}

/**
 * store a block in cache, called by drivers
 */
public bool_t cache_setblock(fid_t fd, blkcnt_t lsn, void *blk, blkprio_t prio)
{
    uint64_t j=lsn%cachelines,bs;
    cache_t *c,*l=NULL,*nc=NULL;
#if DEBUG
    if(_debug&DBG_CACHE)
        dbg_printf("FS: cache_setblock(fd %d, sector %d, buf %x, prio %d)\n",fd,lsn,blk,prio);
#endif
    // failsafe
    if(fd==-1 || fd>=nfcb || lsn==-1 || fcb[fd].type!=FCB_TYPE_DEVICE) return false;
    // block size
    bs=fcb[fd].device.blksize;
    if(bs<=1) return false;
    if((lsn+1)*bs>fcb[fd].device.filesize) { seterr(EFAULT); return false; }
    // already in cache?
    for(c=cache[j];c!=NULL;c=c->next) {
        if(c->fd==fd && c->lsn==lsn) {
            if(l!=NULL)
                l->next=c->next;
            nc=c;
            break;
        }
        l=c;
    }
    // allocate new cache block if not found
    if(nc==NULL) {
        nc=(cache_t*)malloc(sizeof(cache_t));
        if(nc==NULL)
            return false;
        nc->blk=(void*)malloc(bs);
        if(nc->blk==NULL)
            return false;
        nc->fd=fd;
        nc->lsn=lsn;
    }
    // move block in front of the chain
    nc->next=cache[j];
    cache[j]=nc;
    // update cache block
    nc->fd=fd|((uint64_t)(prio*0xF)<<60);
    memcpy(nc->blk,blk,bs);
    return true;
}

/**
 * remove all blocks of a device from cache
 */
public blkcnt_t cache_freeblocks(fid_t fd, blkcnt_t needed)
{
    uint64_t i,j,k;
    blkcnt_t cnt=0;
    cache_t *c,*l,**r=NULL;
#if DEBUG
//    if(_debug&DBG_CACHE)
        dbg_printf("FS: cache_freeblocks(fd %d, needed %d)\n",fd,needed);
#endif
    // failsafe
    if(fd==-1 || fd>=nfcb || fcb[fd].type!=FCB_TYPE_DEVICE || fcb[fd].device.blksize<=1) return 0;
    // assume 512 blocks per cachelines at first, so allocate one page for indexing
    k=512; r=(cache_t**)malloc(k*sizeof(cache_t*));
    if(r==NULL) goto failsafe;
    // reverse order, so the superblock will be freed in the last round
    for(j=cachelines-1;(int)j>=0 && (needed==0 || cnt<needed);j--) {
        if(cache[j]==NULL)
            continue;
        // I decided not to store prev links all the time in a double
        // linked list, so we have to build an index now.
        i=0;
        for(c=cache[j];c!=NULL;c=c->next) {
            r[i++]=c;
            // resize index array if needed
            if(i+1>=k) {
                k<<=1; r=(cache_t**)realloc(r,k*sizeof(cache_t*));
                if(r==NULL) goto failsafe;
            }
        }
        // free up blocks, going in reverse order, so starting with the last recently used one
        for(i--;(int)i>=0;i--) {
            c=r[i];
            if(CACHE_FD(c->fd)==fd) {
                if(i>0)
                    r[i-1]->next=c->next;
                else
                    cache[j]=c->next;
                free(c->blk);
                free(c);
                cnt++;
            }
        }
    }
    free(r);
    return cnt;
    /* failsafe implementation. Does not need extra memory, but it also does not
     * guarantee that last recently used block freed first. But in out of RAM
     * scenario, better than nothing */
failsafe:
    for(j=cachelines-1;(int)j>=0 && (needed==0 || cnt<needed);j--) {
        if(cache[j]==NULL)
            continue;
        l=cache[j];
        for(c=cache[j];c!=NULL;c=c->next) {
            if(CACHE_FD(c->fd)==fd) {
                l->next=c->next;
                l=c->next;
                free(c->blk);
                free(c);
                c=l;
                cnt++;
            }
            l=c;
        }
    }
    return cnt;
}

/**
 * flush cache to drives
 */
void cache_flush()
{
    uint64_t i,k,p;
    fid_t f;
    cache_t *c;
    cache_inflush=0;
    // make sure it's flushed according to soft update priority
    for(p=BLKPRIO_CRIT;p<=BLKPRIO_META;p++)
        for(i=0;i<cachelines;i++) {
            if(cache[i]==NULL)
                continue;
            for(c=cache[i];c!=NULL;c=c->next)
                if(CACHE_PRIO(c->fd)==p) {
                    f=CACHE_FD(c->fd);
                    k=fcb[f].device.inode;
                    // block dirty in cache. Send a message to the driver to write out
                    mq_send(dev[k].drivertask, DRV_write|MSG_PTRDATA, c->blk, fcb[f].device.blksize,
                        dev[k].device, f, c->lsn, k, ctx->pid);
                    dev[k].total++;
                    cache_inflush++;
                }
        }
#if DEBUG
    if(_debug&DBG_CACHE)
        dbg_printf("FS: cache_flush() flushing %d blks\n",cache_inflush);
#endif
}

/**
 * clear dirty flag on cache block and increase written out counter on dev
 */
bool_t cache_cleardirty(fid_t fd, blkcnt_t lsn)
{
    uint64_t i,j=lsn%cachelines;
    cache_t *c;
#if DEBUG
    if(_debug&DBG_CACHE)
        dbg_printf("FS: cache_cleardirty(fd %d, sector %d)\n",fd,lsn);
#endif
    // failsafe
    if(fd==-1 || fd>=nfcb || lsn==-1 || fcb[fd].type!=FCB_TYPE_DEVICE || cache[j]==NULL)
        return false;
    // find it in block chain
    for(c=cache[j];c!=NULL;c=c->next)
        if(CACHE_FD(c->fd)==fd && c->lsn==lsn) {
            c->fd=CACHE_FD(fd);
            break;
        }
    // increase number of blocks written out
    i=fcb[fd].device.inode;
    dev[i].out++;
    if(dev[i].out >= dev[i].total)
        dev[i].out=dev[i].total=0;
    // notify UI to update progressbar on dock icons
    mq_send(SRV_UI, SYS_devprogress, fd, dev[i].out, dev[i].total);
    cache_inflush--;
#if DEBUG
    if(_debug&DBG_CACHE && cache_inflush==0)
        dbg_printf("FS: cache_cleardirty() all blks written out\n");
#endif
    return cache_inflush==0;
}

/**
 * free up at least half of the cache by releasing last recently used blocks
 */
void cache_free()
{
    uint64_t i,k;
    blkcnt_t total=0, freed=0;
    cache_t *c;
    // count the total number of blocks. We rarely need this, so it's
    // acceptable not to keep track of the number all the time
    for(i=0;i<cachelines;i++) {
        if(cache[i]==NULL)
            continue;
        for(c=cache[i];c!=NULL;c=c->next)
            total++;
    }
    // we need to free at least half of it
    total>>=1; i=0;k=-1;
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
        dbg_printf("FS: cache_free() freed %d out of %d blks\n", freed, total<<1);
#endif
}

#if DEBUG
void cache_dump()
{
    uint64_t i;
    cache_t *c;
    blkcnt_t total=0;
    for(i=0;i<cachelines;i++) {
        if(cache[i]==NULL)
            continue;
        for(c=cache[i];c!=NULL;c=c->next)
            total++;
    }
    dbg_printf("Block cache %d:\n",total);
    for(i=0;i<cachelines;i++) {
        if(cache[i]==NULL)
            continue;
        dbg_printf("%3d:",i);
        for(c=cache[i];c!=NULL;c=c->next)
            dbg_printf(" (prio %d, fd %d, lsn %d)",CACHE_PRIO(c->fd),CACHE_FD(c->fd),c->lsn);
        dbg_printf("\n");
    }
}
#endif
