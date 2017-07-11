/*
 * fs/cache.h
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
 * @brief Cache definitions
 */
#include <osZ.h>

#define CACHEDIRNAME 56
typedef struct {
    fid_t fid;
    char name[CACHEDIRNAME];
} cachedir_t;

extern uint64_t ncachedir;
extern cachedir_t *cachedir;

typedef struct {
    fpos_t pos;
    void *cache;
} cacheidx_t;

typedef struct {
    pid_t drivertask;   //major
    dev_t device;       //minor
    size_t idxlen;
    cacheidx_t *idx;
} cache_t;

typedef struct {
    size_t len;
    void *blks;
} cacheblk_t;
#define CBLK_IDX(bs) (bs==512?0:(bs==1024?1:(bs==2048?2:(bs==4096?3:(bs==8192?4:(bs==16384?5:(bs=32768?6:7)))))))

extern uint64_t ncache;
extern cachedir_t *cache;
extern cacheblk_t cacheblk[];

extern void cache_dir(char*name,fid_t fid);
extern void *cache_getblock(dev_t dev,blkcnt_t blk);

#if DEBUG
extern void cache_dump();
#endif
