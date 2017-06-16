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
#include <sys/mman.h>

/***
 * Memory layout:
 * 0. slot: allocmap_t
 * 1. slot: extended allocmap_t area
 * 2. slot: chunk's data (more chunks, if quantum is small)
 * 3. slot: another chunk's data
 * 4. slot: first chunk of a large quantum
 * 5. slot: second chunk of a large quantum
 * ...
 * 
 * Small units (quantum>=8 && quantum<CHUNKSIZE):
 *  ptr points to a page aligned address, bitmap maps quantum sized units
 *
 * Large units (quantum>=CHUNKSIZE):
 *  ptr points to a slot aligned address, size[0] holds number of continous chunks
 *
 * Depends on the following libc functions:
 * - void lockacquire(int bit, uint64_t *ptr);        // return only when the bit is set, yield otherwise
 * - void lockrelease(int bit, uint64_t *ptr);        // release a bit
 * - int bitalloc(int numints,uint64_t *ptr);         // find the first cleared bit and set. Return -1 or error
 * - #define bitfree(b,p) lockrelease(b,p)            // clear a bit, same as lockrelease
 * - void *memset (void *s, int c, size_t n);         // set a memory with character
 * - void *memcpy (void *dest, void *src, size_t n);  // copy memory
 * - void *mmap (void *addr, size_t len, int prot, int flags, -1, 0); //query memory from pmm
 * - int munmap (void *addr, size_t len);                             //give back memory to system
 */
#define CHUNKSIZE     __SLOTSIZE                                //one chunk is 2M
#define ALLOCMAPMAX  (8*__SLOTSIZE)                             //maximum size for chunks array
#define BITMAPSIZE   (CHUNKSIZE/__PAGESIZE/8/8)                 //number of allocation blocks in a chunk
#define numchunks(a) (a->numchunks&0x7ffffffff)                 //number of chunks in an arena
#define chunksize(q) ((q*BITMAPSIZE*64/__PAGESIZE)*__PAGESIZE)  //size of a chunk for a given quantum

typedef struct {
    void *ptr;                      //memory pointer (page or slot aligned)
    uint64_t quantum;               //allocation unit in this chunk
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

/**
 * This allocator is used for both thread local storage and shared memory.
 * Arena points to an allocation map (either at BSS_ADDRESS or SBSS_ADDRESS).
 */
void *bzt_alloc(allocmap_t *arena,size_t a,void *ptr,size_t s,int flag)
{
    uint64_t q;
    int i,j,nc=-1,oc=-1;
    void *fp=arena+ALLOCMAPMAX, *end;
    int prot = PROT_READ | PROT_WRITE;
    flag |= MAP_FIXED | MAP_ANONYMOUS;
    // get quantum
    for(q=8;q<s;q<<=1);

    lockacquire(63,&arena->numchunks);
#if DEBUG
    if(_debug&DBG_MALLOC)
        dbg_printf("bzt_alloc(%x, %x, %x, %d) quantum %d\n", arena, a, ptr, s, q);
#endif
    // get chunk for this quantum size
    for(i=0;i<numchunks(arena);i++) {
        end=arena->chunk[i].ptr + chunksize(arena->chunk[i].quantum);
        if(arena->chunk[i].quantum==q)
            for(j=0;j<BITMAPSIZE;j++)
                if(arena->chunk[i].map[j]!=-1) { nc=i; break; }
        if(ptr!=NULL && arena->chunk[i].ptr <= ptr && ptr < end)
                oc=i;
        if(end > fp)
            fp=end;
    }
    // same as before
    if(ptr!=NULL && nc!=-1 && nc==oc) {
        lockrelease(63,&arena->numchunks);
        return ptr;
    }
    if(nc==-1) {
        // do we have place for a new chunk?
        if((numchunks(arena)+1)*sizeof(chunkmap_t)+8 > ALLOCMAPMAX) {
            seterr(ENOMEM);
            lockrelease(63,&arena->numchunks);
            return NULL;
        }
        // add a new chunk
        nc=numchunks(arena);
        arena->numchunks++;
        // passed page boundary?
        i=(numchunks(arena)*sizeof(chunkmap_t)+8)/__PAGESIZE;
        if( (nc*sizeof(chunkmap_t)+8)/__PAGESIZE < i) {
/*
            if(!mmap(arena+i*__PAGESIZE, __PAGESIZE, prot, flag, -1, 0)) {
                seterr(ENOMEM);
                lockrelease(63,&arena->numchunks);
                return NULL;
            }
*/
        }
        arena->chunk[nc].quantum = q;
        arena->chunk[nc].ptr = fp;
        memset(&arena->chunk[nc].map,0,BITMAPSIZE*8);
        // allocate memory for data
        if(!mmap(fp, chunksize(q), prot, flag, -1, 0)) {
//            seterr(ENOMEM);
            lockrelease(63,&arena->numchunks);
            return NULL;
        }
    }
    i=bitalloc(BITMAPSIZE, arena->chunk[nc].map);
dbg_printf(" arena nc %d oc %d i %d\n",nc,oc,i);
    lockrelease(63,&arena->numchunks);
    if(i>=0) {
        return arena->chunk[nc].ptr + i*arena->chunk[nc].quantum;
    }
    return ptr;
}

void bzt_free(allocmap_t *arena, void *ptr)
{
    lockacquire(63,&arena->numchunks);
#if DEBUG
    if(_debug&DBG_MALLOC)
        dbg_printf("bzt_free(%x, %x)\n", arena, ptr);
#endif
    lockrelease(63,&arena->numchunks);
}

/**
 * dump memory map, for debugging purposes
 */
void bzt_dumpmem(allocmap_t *arena)
{
    int i,j,k,l;
    int mask[]={1,2,4,8,16,32,64,128,256,512,1024,2048,4096,8192,16384,32768};
    char *bmap=".123456789ABCDEF";
    uint16_t *m;
    dbg_printf("-----Arena %x, %s%s, ",
        arena,(uint64_t)arena==(uint64_t)BSS_ADDRESS?"lts":"shared",
        (arena->numchunks & (1UL<<63))!=0?" LOCKED":"");
    dbg_printf("chunks: %d/%d (%d bytes, %d units)\n",
        numchunks(arena), (ALLOCMAPMAX-8)/sizeof(chunkmap_t), CHUNKSIZE, BITMAPSIZE*64);
    for(i=0;i<numchunks(arena);i++) {
        dbg_printf("%3d. %6d %x - %x ",i+1,
            arena->chunk[i].quantum,arena->chunk[i].ptr,arena->chunk[i].ptr+chunksize(arena->chunk[i].quantum));
        m=(uint16_t*)arena->chunk[i].map;
        for(j=0;j<BITMAPSIZE*4;j++) {
            k=0; for(l=0;l<16;l++) { if(*m & mask[l]) k++; }
            dbg_printf("%c",bmap[k]);
            m++;
        }
        dbg_printf("%8dk\n",chunksize(arena->chunk[i].quantum)/1024);
    }
}
