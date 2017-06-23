/*
 * test/mmap.c
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
 * @brief Memory mapping and allocation tests
 */
#include <osZ.h>

void mmap_test()
{
    uint64_t fm;
    meminfo_t m=meminfo();dbg_printf("pmm: %d/%d\n",m.freepages,m.totalpages);
    fm=m.freepages;

    void *ptr=malloc(100);
    dbg_printf("ptr=%x errno=%d\n",ptr, errno);
    m=meminfo();dbg_printf("pmm: %d/%d diff: %d\n",m.freepages,m.totalpages,fm-m.freepages);
#if DEBUG
    if(_debug&DBG_MALLOC)
        bzt_dumpmem((void*)BSS_ADDRESS);
#endif

    void *ptr2=malloc(200);
    dbg_printf("ptr2=%x errno=%d\n",ptr2, errno);
#if DEBUG
    if(_debug&DBG_MALLOC)
        bzt_dumpmem((void*)BSS_ADDRESS);
#endif

    void *ptr3=malloc(100);
    dbg_printf("ptr3=%x errno=%d\n",ptr3, errno);
#if DEBUG
    if(_debug&DBG_MALLOC)
        bzt_dumpmem((void*)BSS_ADDRESS);
#endif

    ptr=realloc(ptr,120);
    dbg_printf("ptr=%x errno=%d\n",ptr, errno);
#if DEBUG
    if(_debug&DBG_MALLOC)
        bzt_dumpmem((void*)BSS_ADDRESS);
#endif

    ptr=realloc(ptr,200);
    dbg_printf("ptr=%x errno=%d\n",ptr, errno);
#if DEBUG
    if(_debug&DBG_MALLOC)
        bzt_dumpmem((void*)BSS_ADDRESS);
#endif

    free(ptr);
#if DEBUG
    if(_debug&DBG_MALLOC)
        bzt_dumpmem((void*)BSS_ADDRESS);
#endif

    free(ptr3);
#if DEBUG
    if(_debug&DBG_MALLOC)
        bzt_dumpmem((void*)BSS_ADDRESS);
#endif

    free(ptr2);
    m=meminfo();dbg_printf("pmm: %d/%d diff: %d\n",m.freepages,m.totalpages,fm-m.freepages);
#if DEBUG
    if(_debug&DBG_MALLOC)
        bzt_dumpmem((void*)BSS_ADDRESS);
#endif

    ptr=malloc(20000);
    dbg_printf("ptr=%x errno=%d\n",ptr, errno);
    m=meminfo();dbg_printf("pmm: %d/%d diff: %d\n",m.freepages,m.totalpages,fm-m.freepages);
#if DEBUG
    if(_debug&DBG_MALLOC)
        bzt_dumpmem((void*)BSS_ADDRESS);
#endif

    ptr2=malloc(__SLOTSIZE);
    dbg_printf("ptr2=%x errno=%d\n",ptr2, errno);
#if DEBUG
    if(_debug&DBG_MALLOC)
        bzt_dumpmem((void*)BSS_ADDRESS);
#endif

    ptr3=malloc(2*__SLOTSIZE);
    dbg_printf("ptr3=%x errno=%d\n",ptr3, errno);
#if DEBUG
    if(_debug&DBG_MALLOC)
        bzt_dumpmem((void*)BSS_ADDRESS);
#endif

    ptr=realloc(ptr,__SLOTSIZE);
    dbg_printf("ptr=%x errno=%d\n",ptr, errno);
    m=meminfo();dbg_printf("pmm: %d/%d diff: %d\n",m.freepages,m.totalpages,fm-m.freepages);
#if DEBUG
    if(_debug&DBG_MALLOC)
        bzt_dumpmem((void*)BSS_ADDRESS);
#endif

    ptr=realloc(ptr,2*__SLOTSIZE);
    dbg_printf("ptr=%x errno=%d\n",ptr, errno);
    m=meminfo();dbg_printf("pmm: %d/%d diff: %d\n",m.freepages,m.totalpages,fm-m.freepages);
#if DEBUG
    if(_debug&DBG_MALLOC)
        bzt_dumpmem((void*)BSS_ADDRESS);
#endif
    free(ptr3);
#if DEBUG
    if(_debug&DBG_MALLOC)
        bzt_dumpmem((void*)BSS_ADDRESS);
#endif

    free(ptr2);
#if DEBUG
    if(_debug&DBG_MALLOC)
        bzt_dumpmem((void*)BSS_ADDRESS);
#endif

    free(ptr);
#if DEBUG
    if(_debug&DBG_MALLOC)
        bzt_dumpmem((void*)BSS_ADDRESS);
#endif
    dbg_printf("freed.\n");

    m=meminfo();dbg_printf("pmm: %d/%d diff: %d\n",m.freepages,m.totalpages,fm-m.freepages);

    ptr=malloc(8);
    dbg_printf("ptr=%x errno=%d align=8\n",ptr, errno);
#if DEBUG
    if(_debug&DBG_MALLOC)
        bzt_dumpmem((void*)BSS_ADDRESS);
#endif
    
    ptr2=aligned_alloc(128,8);
    if((uint64_t)ptr2&127UL)
        dbg_printf("UNALIGNED!!! ");
    dbg_printf("ptr2=%x errno=%d align=128\n",ptr2, errno);
#if DEBUG
    if(_debug&DBG_MALLOC)
        bzt_dumpmem((void*)BSS_ADDRESS);
#endif

    free(ptr);
    free(ptr2);
    dbg_printf("freed.\n");

    ptr=malloc(4096);
    dbg_printf("ptr=%x errno=%d align=4096\n",ptr, errno);
#if DEBUG
    if(_debug&DBG_MALLOC)
        bzt_dumpmem((void*)BSS_ADDRESS);
#endif
    
    ptr2=aligned_alloc(65536,4096);
    if((uint64_t)ptr2&65535UL)
        dbg_printf("UNALIGNED!!! ");
    dbg_printf("ptr2=%x errno=%d align=65536\n",ptr2, errno);
#if DEBUG
    if(_debug&DBG_MALLOC)
        bzt_dumpmem((void*)BSS_ADDRESS);
#endif

    ptr3=aligned_alloc(8*1024*1024,2*1024*1024);
    if((uint64_t)ptr3&(8*1024*1024-1))
        dbg_printf("UNALIGNED!!! ");
    dbg_printf("ptr3=%x errno=%d align=8M\n",ptr3, errno);
#if DEBUG
    if(_debug&DBG_MALLOC)
        bzt_dumpmem((void*)BSS_ADDRESS);
#endif
    ptr=realloc(ptr,2*1024*1024);
    dbg_printf("ptr=%x errno=%d align=2M\n",ptr, errno);
#if DEBUG
    if(_debug&DBG_MALLOC)
        bzt_dumpmem((void*)BSS_ADDRESS);
#endif

    free(ptr);
    dbg_printf("free 1st errno=%d\n",errno);
    free(ptr);
    dbg_printf("free 2nd errno=%d\n",errno);
    free(ptr2);
    free(ptr3);
    dbg_printf("freed.\n");

    m=meminfo();dbg_printf("pmm: %d/%d diff: %d\n",m.freepages,m.totalpages,fm-m.freepages);
    if(fm!=m.freepages) {
        dbg_printf("LEAKING!!! %d pages\n",fm-m.freepages);
    }
}
