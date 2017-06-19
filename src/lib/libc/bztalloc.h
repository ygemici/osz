/*
 * lib/libc/bztalloc.h
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
 * @brief Memory allocator and deallocator header
 */

#define ALLOCSIZE     __SLOTSIZE                                 //one alloc slot is 2M
#define ARENASIZE     (ALLOCSIZE)                                //space for 256k pointers
#define MAXARENA      ((ARENASIZE/sizeof(void*))-1)              //maximum number of arenas
#define BITMAPSIZE    (ALLOCSIZE/__PAGESIZE/sizeof(void*)/8)     //number of allocation blocks in a chunk
#define numallocmaps(a) (a[0]&0x7ffffffff)                       //number of allocation maps in an arena
#define chunksize(q)  ((q*BITMAPSIZE*sizeof(void*)*8/__PAGESIZE)*__PAGESIZE)  //size of a chunk for a given quantum

typedef struct {
    uint64_t quantum;               //allocation unit in this chunk
    void *ptr;                      //memory pointer (page or slot aligned)
    union {
        uint64_t map[BITMAPSIZE];   //free units in this chunk (512 bits)
        uint64_t size[BITMAPSIZE];  //size, only first used
    };
} chunkmap_t;

/* this must work with a zeroed page, hence no magic and zero bit represents free */
typedef struct {
    uint64_t numchunks;             //number of chunkmap_t
    chunkmap_t chunk[0];            //chunks
} allocmap_t;
