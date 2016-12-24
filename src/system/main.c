/*
 * system/main.c
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
 * @brief Common event dispatcher for device drivers, the "system" process
 */

#include <osZ.h>

void dump(uint64_t *a,uint64_t b,uint64_t c)
{
//    __asm__ __volatile__ ( "xchgw %%bx,%%bx; syscall" : : : );
}

void _init(int argc, char **argv)
{
    // this is ughly, but we must reference the page after our
    // data segment as core mapped IDT there. But it's neither in
    // our data segment nor in our bss.
	uint64_t *irq_dispatch_table = (uint64_t*)(
        /*TEXT_ADDRESS*/2*1024*1024 + /*_edata*/2*4096);
	// up to ISR_NUMIRQ * irq_dispatch_table[0] + 1

//	uint i;

//  printf("Hello from userspace");
//    dump(irq_dispatch_table, irq_dispatch_table, irq_dispatch_table[0]);
    while(1) {
        msg_t *work = clrecv();
        __asm__ __volatile__ ( "movq %0, %%rax; xchgw %%bx,%%bx; movq %1, %%rbx" : : "r"(work->evt), "r"(work->arg1) :);
//        if(MSG_FUNC(work->evt) == SYS_IRQ) {
//        }
    }
}
