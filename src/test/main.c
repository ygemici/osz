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
    void *ptr=malloc(100);
    dbg_printf("ptr=%x errno=%d\n",ptr, errno);
    bzt_dumpmem((void*)BSS_ADDRESS);

    void *ptr2=malloc(200);
    dbg_printf("ptr2=%x errno=%d\n",ptr2, errno);
    bzt_dumpmem((void*)BSS_ADDRESS);

    void *ptr3=malloc(100);
    dbg_printf("ptr3=%x errno=%d\n",ptr3, errno);
    bzt_dumpmem((void*)BSS_ADDRESS);

    ptr=realloc(ptr,120);
    dbg_printf("ptr=%x errno=%d\n",ptr, errno);
    bzt_dumpmem((void*)BSS_ADDRESS);

    ptr=realloc(ptr,200);
    dbg_printf("ptr=%x errno=%d\n",ptr, errno);
    bzt_dumpmem((void*)BSS_ADDRESS);
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
