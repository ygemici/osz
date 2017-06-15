/*
 * lib/libc/bztalloc.c
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
 * @brief Memory allocator and deallocator.
 */
#define _BZT_ALLOC
#include <osZ.h>

/***
 * Memory layout:
 * 0. slot: allocmap_t
 * 1. slot: small chunks' data
 * 2. slot: medium chunk's data
 * 3. slot: another small chunk's data
 * 4. slot: large chunk data
 * ...
 * 
 * Small units (quantum>=8 && quantum<__PAGESIZE):
 *  ptr points to a page aligned address, bitmap maps quantum sized units
 * 
 * Medium units (quantum>=__PAGESIZE && quantum<__SLOTSLIZE):
 *  ptr points to a slot aligned address, bitmap maps quantum sized units
 * 
 * Large units (quantum>=__SLOTSIZE):
 *  ptr points to a slot aligned address, size[0] holds number of continous chunks
 *
 * Depends on
 * void lockacquire(int bit, uint64_t *ptr); // return only when the bit is set, yield otherwise
 * void lockrelease(int bit, uint64_t *ptr); // release a bit
 * int bitalloc(int numints,uint64_t *ptr);  // find the first cleared bit and set. Return -1 or error
 * #define bitfree(b,p) lockrelease(b,p)     // clear a bit
 */
#define _ALLOCMAP_MAGIC "AMAP"

#define CHUNKSIZE __SLOTSIZE        //one chunk is 2M
#define BITMAPSIZE (__PAGESIZE/8/8/8)

typedef struct {
    void *ptr;                      //memory pointer (page or slot aligned)
    uint64_t quantum;               //allocation unit in this chunk
    union {
        uint64_t map[BITMAPSIZE];   //free units in this chunk (512 bits)
        uint64_t size[BITMAPSIZE];  //size, only first used
    };
} chunkmap_t;

/* this should start with a zeroed page */
typedef struct {
    uint64_t numchunks;             //number of chunkmap_t
    chunkmap_t chunk[0];            //chunks
} allocmap_t;

#define numchunks(a) (a->numchunks&0x7ffffffff)

/**
 * This allocator is used for both thread local storage and shared memory.
 * Arena points to an allocation map (either at BSS_ADDRESS or SBSS_ADDRESS).
 */
void *bzt_alloc(allocmap_t *arena,size_t a,void *ptr,size_t s)
{
    uint64_t q;
    int i,j,nc=-1,oc=-1;
    void *fp=arena;

    lockacquire(63,&arena->numchunks);
    // get quantum
    for(q=8;q<s;q<<=1);
dbg_printf("malloc %d (%d)\n",s,q);
    // get chunk for this quantum size
    for(i=0;i<numchunks(arena);i++) {
        if(arena->chunk[i].quantum==q)
            for(j=0;j<BITMAPSIZE;j++)
                if(arena->chunk[i].map[j]!=-1) { nc=i; break; }
        if(ptr!=NULL && 
            arena->chunk[i].ptr <= ptr && 
            ptr < arena->chunk[i].ptr+CHUNKSIZE)
                oc=i;
        if(arena->chunk[i].ptr > fp)
            fp=arena->chunk[i].ptr;
    }
    // same as before
    if(ptr!=NULL && nc!=-1 && nc==oc) {
        lockrelease(63,&arena->numchunks);
        return ptr;
    }
    if(nc==-1) {
        // add a new chunk
        nc=numchunks(arena);
        arena->numchunks++;
        arena->chunk[nc].quantum = q;
        arena->chunk[nc].ptr = fp+CHUNKSIZE;
        memset(&arena->chunk[nc].map,0,BITMAPSIZE*8);
    }
    i=bitalloc(BITMAPSIZE,arena->chunk[nc].map);
    lockrelease(63,&arena->numchunks);
    if(i>=0) {
        return arena->chunk[nc].ptr + i*arena->chunk[nc].quantum;
    }
    return ptr;
}

void bzt_free(allocmap_t *arena, void *ptr)
{
}
