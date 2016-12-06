/*
 * x86_64/isr.c
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
 * @brief Interrupt Service Routines
 */


#include "../core.h"
#include "../pmm.h"
#include "tcb.h"
#include "ccb.h"
#include "isr.h"

extern void *isr_exc00();
extern void *isr_initgates(uint64_t *idt, OSZ_ccb *tss);
extern void *pmm_alloc();

void isr_init()
{
    OSZ_ccb *ccb = kalloc(1);       //allocate CPU Control Block
    uint64_t *idt = kalloc(1);      //allocate Interrupt Descriptor Table
    uint64_t *safestack = kalloc(1);//allocate extra stack for ISRs
    OSZ_tcb *tcb = 0;               //normal ISRs stack at top of TCB
    void *ptr;
    int i;

    // allocate and map Thread Control Block at offset 0
    kmap((uint64_t)0, (uint64_t)pmm_alloc(),PG_CORE_NOCACHE);
    tcb->magic = OSZ_TCB_MAGICH;
    tcb->state = tcb_running;
    tcb->priority = PRI_SYS;

    // CPU Control Block (TSS64 in kernel bss)
    ccb->magic=OSZ_CCB_MAGICH;
    //usr stack (userspace, first page)
    ccb->ist1=__PAGESIZE;
    //nmi stack (kernel, top 256 bytes of a page)
    ccb->ist2=(uint64_t)safestack+(uint64_t)__PAGESIZE;
    //irq stack (kernel, rest of that page)
    ccb->ist3=(uint64_t)safestack+(uint64_t)__PAGESIZE-256;

    // generate IDT
kprintf("idt=%x isr_exc00=%x\n",idt,isr_exc00);
    ptr=&isr_exc00;
    // 0-31 exception handlers
    // 32-255 irq handlers
    for(i=0;i<32;i++) {
        idt[i*2+0] = IDT_GATE_LO(
            i<32? (i==2||i==8?IDT_NMI:IDT_EXC) : IDT_INT,
            ptr);
        idt[i*2+1] = IDT_GATE_HI(ptr);
        ptr+=ISR_MAX;
    }

    // finally set up isr_syscall dispatcher and IDTR
    isr_initgates(idt, ccb);
}
