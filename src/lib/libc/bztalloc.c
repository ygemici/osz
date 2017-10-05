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
#include <osZ.h>
#ifndef _OS_Z_
#include <sys/mman.h>
#endif
#include "bztalloc.h"

/***
 * Design considerations:
 * 1. don't mix allocator data with application data
 * 2. give back free memory to OS as soon as possible, don't waste resources
 * 3. common allocator for both TLS and shared memory (hence the locking)
 *
 * Memory layout:
 * 0. slot: allocmap_t pointers
 * 1. slot: first allocmap_t
 * 2. slot: chunk's data (more chunks, if quantum is small)
 * 3. slot: second allocmap_t area
 * 4. slot: another chunk's data
 * 5. slot: first chunk of a large quantum
 * 6. slot: second chunk of a large quantum
 * ...etc.
 * 
 * Small units (quantum>=8 && quantum<PAGESIZE):
 *  ptr points to a page aligned address, bitmap maps quantum sized units, allocation done by chunksize
 *
 * Medium units (quantum>=PAGESIZE && quantum<ALLOCSIZE):
 *  ptr points to a page aligned address, bitmap maps quantum sized units, allocation done by quantum
 * 
 * Large units (quantum>=ALLOCSIZE):
 *  ptr points to a slot aligned address, size[0] holds number of continous allocsize blocks
 *
 * Depends on the following libc functions:
 * - void seterr(errno);                              // set errno (EFAULT, ENOMEM)
 * - void lockacquire(int bit, uint64_t *ptr);        // return only when the bit became set, yield otherwise
 * - void lockrelease(int bit, uint64_t *ptr);        // clear a bit
 * - void memzero (void *dest, size_t n);             // clear memory, fill up with zeros
 * - void *memcpy (void *dest, void *src, size_t n);  // copy memory
 * - void *mmap (void *addr, size_t len, int prot, int flags, -1, 0); //query RAM from pmm
 * - int munmap (void *addr, size_t len);                             //give back RAM to system
 * 
 * Provides:
 *   #define malloc(s)          bzt_alloc((void*)BSS_ADDRESS,8,NULL,s,MAP_PRIVATE)
 *   #define calloc(n,s)        bzt_alloc((void*)BSS_ADDRESS,8,NULL,n*s,MAP_PRIVATE)
 *   #define realloc(p,s)       bzt_alloc((void*)BSS_ADDRESS,8,p,s,MAP_PRIVATE)
 *   #define aligned_alloc(a,s) bzt_alloc((void*)BSS_ADDRESS,a,NULL,s,MAP_PRIVATE)
 *   #define free(p)            bzt_free ((void*)BSS_ADDRESS,p)
 */

/**
 * In OS/Z arena points to either at BSS_ADDRESS (TLS) or SBSS_ADDRESS (shared).
 * The argument arena is an array of *allocmap_t, first element records size and lock flag.
 * 
 * free memory and give back RAM to system if possible
 */
void bzt_free(uint64_t *arena, void *ptr)
{
    int i,j,k,cnt=0;
    uint64_t l,sh;
    allocmap_t *am;
    chunkmap_t *ocm=NULL; //old chunk map

    if(ptr==NULL)
        return;

    lockacquire(63,arena);
#if DEBUG
    if(_debug&DBG_MALLOC)
        dbg_printf("bzt_free(%x, %x)\n", arena, ptr);
#endif
    // iterate through allocmaps
    for(i=1;i<MAXARENA && cnt<numallocmaps(arena);i++) {
        if(arena[i]==0)
            continue;
        cnt++;
        // get chunk for this quantum size
        am=(allocmap_t*)arena[i];
        for(j=0;j<am->numchunks;j++) {
            if( am->chunk[j].ptr <= ptr && 
                ptr < am->chunk[j].ptr +
                (am->chunk[j].quantum<ALLOCSIZE? chunksize(am->chunk[j].quantum) : am->chunk[j].map[0])) {
                    ocm=&am->chunk[j];
                    l=0;
                    if(ocm->quantum<ALLOCSIZE) {
                        sh=(ptr-ocm->ptr)/ocm->quantum;
                        ocm->map[sh>>6] &= ~(1UL<<(sh%64));
                        //was it the last allocation in this chunk?
                        l=0; for(k=0;k<BITMAPSIZE;k++) l+=ocm->map[k];
                        if(ocm->quantum<__PAGESIZE) {
                            if(l==0)
                                munmap(ocm->ptr,chunksize(ocm->quantum));
                        } else {
                            munmap(ptr,ocm->quantum);
                        }
                    } else {
                        munmap(ocm->ptr,ocm->map[0]);
                    }
                    if(l==0) {
                        //not last chunk?
                        if(j+1<am->numchunks) {
                            memcpy( &((allocmap_t*)arena[i])->chunk[j],
                                    &((allocmap_t*)arena[i])->chunk[j+1],
                                    (((allocmap_t*)arena[i])->numchunks-j-1)*sizeof(chunkmap_t));
                        }
                        ((allocmap_t*)arena[i])->numchunks--;
                        //was it the last chunk in this allocmap?
                        if(((allocmap_t*)arena[i])->numchunks==0) {
                            munmap((void *)arena[i], ALLOCSIZE);
                            arena[i]=0;
                            arena[0]--;
                        }
                    }
                    lockrelease(63,arena);
                    return;
            }
        }
    }
    lockrelease(63,arena);
    seterr(EFAULT);
}

/**
 * The argument arena is an array of *allocmap_t, first element records size and lock flag.
 *
 * Allocate aligned memory in a memory arena
 */
void *bzt_alloc(uint64_t *arena,size_t a,void *ptr,size_t s,int flag)
{
    uint64_t q,sh,sf;
    int i,j,k,l,fc=0,cnt=0,ocmi=0,ocmj=0;
    void *fp, *sp, *lp, *end;
    allocmap_t *am;       //allocation map
    chunkmap_t *ncm=NULL; //new chunk map
    chunkmap_t *ocm=NULL; //old chunk map
    uint64_t maxchunks = (ALLOCSIZE-8)/sizeof(chunkmap_t);
#ifndef _OS_Z_
    int prot = PROT_READ | PROT_WRITE;
#endif
    flag |= MAP_FIXED | MAP_ANONYMOUS;

    if(s==0) {
        bzt_free(arena,ptr);
        return NULL;
    }

    // get quantum
    for(q=8;q<s;q<<=1);
    // minimum alignment is quantum
    if(a<q) a=q;
    // shifts
    sh=-1; sf=a/q;

    lockacquire(63,arena);
#if DEBUG
    if(_debug&DBG_MALLOC)
        dbg_printf("bzt_alloc(%x, %x, %x, %d) quantum %d\n", arena, a, ptr, s, q);
#endif
    // if we don't have any allocmaps yet
    if(!numallocmaps(arena)) {
        if(
#ifndef _OS_Z_
          mmap((void*)((uint64_t)arena+ARENASIZE), ALLOCSIZE, prot, flag&~MAP_SPARE, -1, 0)
#else
          mmap((void*)((uint64_t)arena+ARENASIZE), ALLOCSIZE, flag&~MAP_SPARE)
#endif
          ==MAP_FAILED) {
            seterr(ENOMEM);
            lockrelease(63,arena);
            return NULL;
        }
        arena[0]++;
        arena[1]=(uint64_t)arena+ARENASIZE;
    }
    fp=(void*)((uint64_t)arena+ARENASIZE+ALLOCSIZE);
    sp=lp=NULL;
    // iterate through allocmaps
    for(i=1;i<MAXARENA && cnt<numallocmaps(arena);i++) {
        if(arena[i]==0)
            continue;
        cnt++;
        // get chunk for this quantum size
        am=(allocmap_t*)arena[i];
        if(((void*)am)+ALLOCSIZE > fp) 
            fp = ((void*)am)+ALLOCSIZE;
        if(lp==NULL || ((void*)am)+ALLOCSIZE < lp) 
            lp = ((void*)am)+ALLOCSIZE;
        if(fc==0 && am->numchunks<maxchunks)
            fc=i;
        for(j=0;j<am->numchunks;j++) {
            end=am->chunk[j].ptr +
                (am->chunk[j].quantum<ALLOCSIZE? chunksize(am->chunk[j].quantum) : am->chunk[j].map[0]);
            if(am->chunk[j].quantum==q && q<ALLOCSIZE && ((uint64_t)(am->chunk[j].ptr)&(a-1))==0) {
                for(k=0;k<BITMAPSIZE*64;k+=sf) {
                    if(((am->chunk[j].map[k>>6])&(1UL<<(k%64)))==0) { ncm=&am->chunk[j]; sh=k; break; }
                }
            }
            if(ptr!=NULL && am->chunk[j].ptr <= ptr && ptr < end) {
                    ocm=&am->chunk[j];
                    ocmi=i; ocmj=j;
            }
            if(sp==NULL || am->chunk[j].ptr < sp)
                sp=am->chunk[j].ptr;
            if(end > fp)
                fp=end;
        }
    }
    // same as before, new size fits in quantum
    if(q<ALLOCSIZE && ptr!=NULL && ncm!=NULL && ncm==ocm) {
        lockrelease(63,arena);
        return ptr;
    }
    // if we have a big enough, aligned space after allocmap, use that area
    if(lp!=NULL && sp!=NULL && 
        lp+((s+ALLOCSIZE-1)/ALLOCSIZE)*ALLOCSIZE < sp && 
        ((uint64_t)lp&(a-1))==0)
            fp=lp;
    if(ncm==NULL) {
        // first allocmap with free chunks
        if(fc==0) {
            //do we have place for a new allocmap?
            if(i>=numallocmaps(arena) ||
#ifndef _OS_Z_
                mmap(fp, ALLOCSIZE, prot, flag&~MAP_SPARE, -1, 0)
#else
                mmap(fp, ALLOCSIZE, flag&~MAP_SPARE)
#endif
                ==MAP_FAILED) {
                    seterr(ENOMEM);
                    lockrelease(63,arena);
                    return ptr;
            }
            arena[0]++;
            //did we just passed a page boundary?
            if((arena[0]*sizeof(void*)/__PAGESIZE)!=((arena[0]+1)*sizeof(void*)/__PAGESIZE)) {
                if(
#ifndef _OS_Z_
                mmap(&arena[arena[0]], __PAGESIZE, prot, flag&~MAP_SPARE, -1, 0)
#else
                mmap(&arena[arena[0]], __PAGESIZE, flag&~MAP_SPARE)
#endif
                ==MAP_FAILED) {
                    seterr(ENOMEM);
                    lockrelease(63,arena);
                    return ptr;
                }
            }
            arena[i]=(uint64_t)fp;
            fp+=ALLOCSIZE;
            fc=i;
        }
        // add a new chunk
        am=(allocmap_t*)arena[fc];
        if(q>=ALLOCSIZE)
            fp=(void*)((((uint64_t)fp+a-1)/a)*a);
        i=am->numchunks++;
        am->chunk[i].quantum = q;
        am->chunk[i].ptr = fp;
        for(k=0;k<BITMAPSIZE;k++)
            am->chunk[i].map[k]=0;
        // allocate memory for small quantum sizes
        if((flag&MAP_SPARE)==0 && q<__PAGESIZE &&
#ifndef _OS_Z_
        mmap(fp, chunksize(q), prot, flag, -1, 0)
#else
        mmap(fp, chunksize(q), flag)
#endif
        ==MAP_FAILED) {
            seterr(ENOMEM);
            lockrelease(63,arena);
            return ptr;
        }
        ncm=&am->chunk[i];
        sh=0;
    }
    if(q<ALLOCSIZE) {
        if(sh!=-1) {
            ncm->map[sh>>6] |= (1UL<<(sh%64));
            fp=ncm->ptr + sh*ncm->quantum;
            //allocate new memory for medium quantum sizes
            if((flag&MAP_SPARE)==0 && q>=__PAGESIZE && 
#ifndef _OS_Z_
            mmap(fp, ncm->quantum, prot, flag, -1, 0)
#else
            mmap(fp, ncm->quantum, flag)
#endif
            ==MAP_FAILED)
                fp=NULL;
        } else
            fp=NULL;
    } else {
        s=((s+ALLOCSIZE-1)/ALLOCSIZE)*ALLOCSIZE;
        //allocate new memory for large quantum sizes
        if((flag&MAP_SPARE)==0 && 
#ifndef _OS_Z_
        mmap(fp, s, prot, flag, -1, 0)
#else
        mmap(fp, s, flag)
#endif
        ==MAP_FAILED)
            fp=NULL;
        else
            ncm->map[0]=s;
    }
    if(fp==NULL){
        seterr(ENOMEM);
        lockrelease(63,arena);
        return ptr;
    }
    if((flag&MAP_SPARE)==0 && ocm==NULL)
        memzero(fp,ncm->quantum);
    //free old memory
    if(ocm!=NULL) {
        if((flag&MAP_SPARE)==0){
            memcpy(fp,ptr,ocm->quantum);
            if(ncm->quantum>ocm->quantum)
                memzero(fp+ocm->quantum,ncm->quantum-ocm->quantum);
        }
        l=0;
        if(ocm->quantum<ALLOCSIZE) {
            sh=(ptr-ocm->ptr)/ocm->quantum;
            ocm->map[sh>>6] &= ~(1UL<<(sh%64));
            //was it the last allocation in this chunk?
            l=0; for(k=0;k<BITMAPSIZE;k++) l+=ocm->map[k];
            if(ocm->quantum<__PAGESIZE) {
                if(l==0)
                    munmap(ocm->ptr,chunksize(ocm->quantum));
            } else {
                munmap(ptr,ocm->quantum);
            }
        } else {
            munmap(ocm->ptr,ocm->map[0]);
        }
        if(l==0) {
            //not last chunk?
            if(ocmj+1<am->numchunks) {
                memcpy( &((allocmap_t*)arena[ocmi])->chunk[ocmj],
                        &((allocmap_t*)arena[ocmi])->chunk[ocmj+1],
                        (((allocmap_t*)arena[ocmi])->numchunks-ocmj-1)*sizeof(chunkmap_t));
            }
            ((allocmap_t*)arena[ocmi])->numchunks--;
            //was it the last chunk in this allocmap?
            if(((allocmap_t*)arena[ocmi])->numchunks==0) {
                munmap((void *)arena[ocmi], ALLOCSIZE);
                arena[ocmi]=0;
                arena[0]--;
            }
        }
    }
    lockrelease(63,arena);
    return fp;
}

/**
 * dump memory map, for debugging purposes
 */
#if DEBUG
void bzt_dumpmem(uint64_t *arena)
{
    int i,j,k,l,n,o,cnt;
    int mask[]={1,2,4,8,16,32,64,128,256,512,1024,2048,4096,8192,16384,32768};
    char *bmap=".123456789ABCDEF";
    uint16_t *m;
    dbg_printf("-----Arena %x, %s%s, ",
        arena,(uint64_t)arena==(uint64_t)BSS_ADDRESS?"lts":"shared",
        (arena[0] & (1UL<<63))!=0?" LOCKED":"");
    dbg_printf("allocmaps: %d/%d, chunks: %d, %d units\n",
        numallocmaps(arena), MAXARENA, (ALLOCSIZE-8)/sizeof(chunkmap_t), BITMAPSIZE*64);
    o=1; cnt=0;
    for(i=1;i<MAXARENA && cnt<numallocmaps(arena);i++) {
        if(arena[i]==0)
            continue;
        cnt++;
        dbg_printf("  --allocmap %d %x, chunks: %d--\n",i,arena[i],((allocmap_t*)arena[i])->numchunks);
        for(j=0;j<((allocmap_t*)arena[i])->numchunks;j++) {
            dbg_printf("%3d. %9d %x - %x ",o++,
                ((allocmap_t*)arena[i])->chunk[j].quantum,
                ((allocmap_t*)arena[i])->chunk[j].ptr,
                ((allocmap_t*)arena[i])->chunk[j].ptr+
                    (((allocmap_t*)arena[i])->chunk[j].quantum<ALLOCSIZE?
                        chunksize(((allocmap_t*)arena[i])->chunk[j].quantum) : 
                        ((allocmap_t*)arena[i])->chunk[j].map[0]));
            if(((allocmap_t*)arena[i])->chunk[j].quantum<ALLOCSIZE) {
                m=(uint16_t*)&((allocmap_t*)arena[i])->chunk[j].map;
                for(k=0;k<BITMAPSIZE*4;k++) {
                    l=0; for(n=0;n<16;n++) { if(*m & mask[n]) l++; }
                    dbg_printf("%c",bmap[l]);
                    m++;
                }
                dbg_printf("%8dk\n",chunksize(((allocmap_t*)arena[i])->chunk[j].quantum)/1024);
            } else {
                dbg_printf("(no bitmap) %8dM\n",((allocmap_t*)arena[i])->chunk[j].map[0]/1024/1024);
            }
        }
    }
    dbg_printf("\n");
}
#endif
