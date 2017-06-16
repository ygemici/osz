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
 * 0. slot: array of allocmap_t pointers
 * 1. slot: first allocmap_t
 * 2. slot: chunk's data (more chunks, if quantum is small)
 * 3. slot: second allocmap_t area
 * 4. slot: another chunk's data
 * 5. slot: first chunk of a large quantum
 * 6. slot: second chunk of a large quantum
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
#define MAXARENA     ((CHUNKSIZE/sizeof(void*))-1)              //maximum number of arenas
#define BITMAPSIZE   (CHUNKSIZE/__PAGESIZE/sizeof(void*)/8)     //number of allocation blocks in a chunk
#define numallocmaps(a) (a[0]&0x7ffffffff)                         //number of chunks in an arena
#define chunksize(q) ((q*BITMAPSIZE*sizeof(void*)*8/__PAGESIZE)*__PAGESIZE)  //size of a chunk for a given quantum

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

/**
 * arena is an array of *allocmap_t, first element records size and lock flag
 */

/**
 * This allocator is used for both thread local storage and shared memory.
 * Arena points to an allocation map (either at BSS_ADDRESS or SBSS_ADDRESS).
 */
void *bzt_alloc(uint64_t *arena,size_t a,void *ptr,size_t s,int flag)
{
    uint64_t q;
    int i,j,k,fc=0;
    void *fp, *end;
    allocmap_t *am;
    chunkmap_t *ncm=NULL; //new chunk map
    chunkmap_t *ocm=NULL; //old chunk map
    uint64_t maxchunks = (CHUNKSIZE-8)/sizeof(chunkmap_t);
    int prot = PROT_READ | PROT_WRITE;
    flag |= MAP_FIXED | MAP_ANONYMOUS;

    // get quantum
    for(q=8;q<s;q<<=1);

    lockacquire(63,arena);
#if DEBUG
    if(_debug&DBG_MALLOC)
        dbg_printf("bzt_alloc(%x, %x, %x, %d) quantum %d\n", arena, a, ptr, s, q);
#endif
    // if we don't have any allocmaps
    if(!numallocmaps(arena)) {
        if(mmap((void*)((uint64_t)arena+CHUNKSIZE), CHUNKSIZE, prot, flag, -1, 0)==MAP_FAILED) {
            seterr(ENOMEM);
            lockrelease(63,arena);
            return NULL;
        }
        arena[0]++;
        arena[1]=(uint64_t)arena+CHUNKSIZE;
        fp=(void*)((uint64_t)arena+2*CHUNKSIZE);
    }
    // iterate through allocmaps
    for(i=1;i<MAXARENA && arena[i]!=0;i++) {
        // get chunk for this quantum size
        am=(allocmap_t*)arena[i];
dbg_printf("am %x %d\n",am, fc);
        if(fc==0 && am->numchunks<maxchunks)
            fc=i;
        for(j=0;j<am->numchunks;j++) {
            end=am->chunk[j].ptr + chunksize(am->chunk[j].quantum);
            if(am->chunk[j].quantum==q)
                for(k=0;k<BITMAPSIZE;k++)
                    if(am->chunk[j].map[k]!=-1) { ncm=&am->chunk[j]; break; }
            if(ptr!=NULL && am->chunk[j].ptr <= ptr && ptr < end)
                    ocm=&am->chunk[j];
            if(end > fp)
                fp=end;
        }
dbg_printf("fp %x\n", fp);
    }
    // same as before, new size fits in quantum
    if(ptr!=NULL && ncm!=NULL && ncm==ocm) {
        lockrelease(63,arena);
        return ptr;
    }
    if(ncm==NULL) {
        // first allocmap with free chunks
        if(fc==0) {
            //do we have place for a new allocmap?
            if(i>=numallocmaps(arena) ||
                mmap(fp, CHUNKSIZE, prot, flag, -1, 0)==MAP_FAILED) {
                    seterr(ENOMEM);
                    lockrelease(63,arena);
                    return NULL;
            }
            arena[0]++;
            arena[i]=(uint64_t)fp;
            fp+=CHUNKSIZE;
            fc=i;
        }
        // add a new chunk
        am=(allocmap_t*)arena[fc];
        i=am->numchunks++;
        am->chunk[i].quantum = q;
        am->chunk[i].ptr = fp;
        memset(&am->chunk[i].map,0,BITMAPSIZE*8);
        // allocate memory for data
        if(!mmap(fp, chunksize(q), prot, flag, -1, 0)) {
            seterr(ENOMEM);
            lockrelease(63,arena);
            return NULL;
        }
        ncm=&am->chunk[i];
    }
    i=bitalloc(BITMAPSIZE, (uint64_t*)&ncm->map);
dbg_printf("   nc %x oc %x i %d\n",ncm,ocm,i);
    lockrelease(63,arena);
    if(i>=0 && i<BITMAPSIZE*64) {
        fp=ncm->ptr + i*ncm->quantum;
        //free old memory
        if(ocm!=NULL) {
dbg_printf(" release %x %d to %x\n",ptr,ocm->quantum,fp);
            memcpy(fp,ptr,ocm->quantum);
            bitfree((ptr-ocm->ptr)/ocm->quantum, (uint64_t*)&ncm->map);
        }
        return fp;
    }
    return ptr;
}

void bzt_free(uint64_t *arena, void *ptr)
{
    lockacquire(63,arena);
#if DEBUG
    if(_debug&DBG_MALLOC)
        dbg_printf("bzt_free(%x, %x)\n", arena, ptr);
#endif
    lockrelease(63,arena);
}

/**
 * dump memory map, for debugging purposes
 */
#if DEBUG
void bzt_dumpmem(uint64_t *arena)
{
    int i,j,k,l,n,o;
    int mask[]={1,2,4,8,16,32,64,128,256,512,1024,2048,4096,8192,16384,32768};
    char *bmap=".123456789ABCDEF";
    uint16_t *m;
    dbg_printf("-----Arena %x, %s%s, ",
        arena,(uint64_t)arena==(uint64_t)BSS_ADDRESS?"lts":"shared",
        (arena[0] & (1UL<<63))!=0?" LOCKED":"");
    dbg_printf("allocmaps: %d/%d, chunks: %d, %d units\n",
        numallocmaps(arena), MAXARENA, (CHUNKSIZE-8)/sizeof(chunkmap_t), BITMAPSIZE*64);
    o=1;
    for(i=1;i<MAXARENA && arena[i]!=0;i++) {
        dbg_printf("  --allocmap %d %x, chunks: %d--\n",i,arena[i],((allocmap_t*)arena[i])->numchunks);
        for(j=0;j<((allocmap_t*)arena[i])->numchunks;j++) {
            dbg_printf("%3d. %6d %x - %x ",o++,
                ((allocmap_t*)arena[i])->chunk[j].quantum,
                ((allocmap_t*)arena[i])->chunk[j].ptr,
                ((allocmap_t*)arena[i])->chunk[j].ptr+chunksize(((allocmap_t*)arena[i])->chunk[j].quantum));
            if(((allocmap_t*)arena[i])->chunk[j].quantum<CHUNKSIZE) {
                m=(uint16_t*)&((allocmap_t*)arena[i])->chunk[j].map;
                for(k=0;k<BITMAPSIZE*4;k++) {
                    l=0; for(n=0;n<16;n++) { if(*m & mask[n]) l++; }
                    dbg_printf("%c",bmap[l]);
                    m++;
                }
                dbg_printf("%8dk\n",chunksize(((allocmap_t*)arena[i])->chunk[j].quantum)/1024);
            } else {
                dbg_printf("%8dM\n",((allocmap_t*)arena[i])->chunk[j].size[0]/1024/1024);
            }
        }
    }
}
#endif
