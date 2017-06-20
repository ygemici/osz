/*
 * test/main.c
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
 * @brief Application to test services and libraries
 */
#include <osZ.h>

void mmap_test()
{
    uint64_t fm;
    msg_t *msg=meminfo();
    dbg_printf("pmm: %d/%d\n",msg->arg0,msg->arg1);
    fm=msg->arg0;

    void *ptr=malloc(100);
    dbg_printf("ptr=%x errno=%d\n",ptr, errno);
    msg=meminfo();
    dbg_printf("pmm: %d/%d\n",msg->arg0,msg->arg1);
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
    msg=meminfo();
    dbg_printf("pmm: %d/%d\n",msg->arg0,msg->arg1);
    dbg_printf("pmm diff: %d\n",fm-msg->arg0);
#if DEBUG
    if(_debug&DBG_MALLOC)
        bzt_dumpmem((void*)BSS_ADDRESS);
#endif

    ptr=malloc(20000);
    dbg_printf("ptr=%x errno=%d\n",ptr, errno);
    msg=meminfo();
    dbg_printf("pmm: %d/%d\n",msg->arg0,msg->arg1);
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
    msg=meminfo();
    dbg_printf("pmm: %d/%d\n",msg->arg0,msg->arg1);
#if DEBUG
    if(_debug&DBG_MALLOC)
        bzt_dumpmem((void*)BSS_ADDRESS);
#endif

    ptr=realloc(ptr,2*__SLOTSIZE);
    dbg_printf("ptr=%x errno=%d\n",ptr, errno);
    msg=meminfo();
    dbg_printf("pmm: %d/%d\n",msg->arg0,msg->arg1);
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

    msg=meminfo();
    dbg_printf("pmm: %d/%d\n",msg->arg0,msg->arg1);
    dbg_printf("pmm diff: %d\n",fm-msg->arg0);

    ptr=malloc(8);
    dbg_printf("ptr=%x errno=%d\n",ptr, errno);
#if DEBUG
    if(_debug&DBG_MALLOC)
        bzt_dumpmem((void*)BSS_ADDRESS);
#endif
    
    ptr2=aligned_alloc(128,8);
    if((uint64_t)ptr2&127UL)
        dbg_printf("UNALIGNED!!! ");
    dbg_printf("ptr2=%x errno=%d\n",ptr2, errno);
#if DEBUG
    if(_debug&DBG_MALLOC)
        bzt_dumpmem((void*)BSS_ADDRESS);
#endif

    free(ptr);
    free(ptr2);

    ptr=malloc(4096);
    dbg_printf("ptr=%x errno=%d\n",ptr, errno);
#if DEBUG
    if(_debug&DBG_MALLOC)
        bzt_dumpmem((void*)BSS_ADDRESS);
#endif
    
    ptr2=aligned_alloc(65536,4096);
    if((uint64_t)ptr2&65535UL)
        dbg_printf("UNALIGNED!!! ");
    dbg_printf("ptr2=%x errno=%d\n",ptr2, errno);
#if DEBUG
    if(_debug&DBG_MALLOC)
        bzt_dumpmem((void*)BSS_ADDRESS);
#endif

    ptr3=aligned_alloc(8*1024*1024,2*1024*1024);
    if((uint64_t)ptr3&(8*1024*1024-1))
        dbg_printf("UNALIGNED!!! ");
    dbg_printf("ptr3=%x errno=%d\n",ptr3, errno);
#if DEBUG
    if(_debug&DBG_MALLOC)
        bzt_dumpmem((void*)BSS_ADDRESS);
#endif
    ptr=realloc(ptr,2*1024*1024);
    dbg_printf("ptr=%x errno=%d\n",ptr, errno);
#if DEBUG
    if(_debug&DBG_MALLOC)
        bzt_dumpmem((void*)BSS_ADDRESS);
#endif

    free(ptr);
    free(ptr2);
    free(ptr3);

}

int main(int argc, char**argv)
{
    //wait until sys_ready() sends us an SYS_ack message
    mq_recv();
    dbg_printf("\n------------------------- TESTS ----------------------------\n");
    
    //do tests
    mmap_test();

    dbg_printf("------------------------- TESTS END ----------------------------\n\n");

    //do nothing
    while(1) mq_recv();
    return 0;
}
