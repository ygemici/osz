/*
 * core/x86_64/isr.c
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


#include "platform.h"
#include "isr.h"
#include "ccb.h"

extern void isr_exc00divzero();
extern void isr_irq0();
extern void *isr_initgates(uint64_t *idt, OSZ_ccb *tss);
extern OSZ_ccb ccb;
extern pid_t subsystems[];

uint64_t __attribute__ ((section (".data"))) identity_mapping;

/* Initialize interrupts */
void isr_init()
{
    uint64_t *idt = kalloc(1);      //allocate Interrupt Descriptor Table
    uint64_t *safestack = kalloc(1);//allocate extra stack for ISRs
    void *ptr;
    int i;

    // CPU Control Block (TSS64 in kernel bss)
    kmap((uint64_t)&ccb, (uint64_t)pmm_alloc(), PG_CORE_NOCACHE);
    ccb.magic = OSZ_CCB_MAGICH;
    //usr stack (userspace, first page)
    ccb.ist1 = __PAGESIZE;
    //nmi stack (kernel, top 256 bytes of a page)
    ccb.ist2 = (uint64_t)safestack + (uint64_t)__PAGESIZE;
    //irq stack (kernel, rest of that page)
    ccb.ist3 = (uint64_t)safestack + (uint64_t)__PAGESIZE-256;

    // generate IDT
    ptr = &isr_exc00divzero;
    // 0-31 exception handlers
    for(i=0;i<32;i++) {
        idt[i*2+0] = IDT_GATE_LO(i==2||i==8?IDT_NMI:IDT_EXC, ptr);
        idt[i*2+1] = IDT_GATE_HI(ptr);
        ptr+=ISR_MAX;
    }
    // 32-255 irq handlers
    ptr = &isr_irq0;
    for(i=32;i<64;i++) {
        idt[i*2+0] = IDT_GATE_LO(IDT_INT, ptr);
        idt[i*2+1] = IDT_GATE_HI(ptr);
        ptr+=ISR_MAX;
    }

    // finally set up isr_syscall dispatcher and IDTR
    isr_initgates(idt, &ccb);
}

void isr_enable()
{
    OSZ_tcb *tcb = (OSZ_tcb*)0;
    // map "system" process
    thread_map(subsystems[SRV_system]);
// TODO: remove test
msg_sendsys(SYS_IRQ,2,0,0);
msg_sendsys(5,6,7,8);
    // start interrupts, fake an interrupt
    // handler return to start multitasking
    __asm__ __volatile__ (
        "movq %0, %%rsp; movq %1, %%rbp; iretq" :
        :
        "b"(&tcb->rip), "i"(TEXT_ADDRESS) :
        "%rsp" );
}

void isr_disable()
{
      // clear interrupts and identity map memory
      // (required by ACPI parser to power off machine)
      __asm__ __volatile__ ( "cli; mov %0, %%rax; mov %%rax, %%cr3" : : "r"(identity_mapping) : "%rax" );
}

/* common IRQ handler */
void isr_irq(uint64_t irq)
{
    kprintf("---- interrupt %d -----\n",irq);
}

/* fallback exception handler */
void excabort(uint64_t excno, uint64_t errcode)
{
    kpanic("---- exception %d ----",excno);
}

/* exception specific code */
void exc00divzero(uint64_t excno)
{
    kpanic("divzero %d",excno);
}

void exc01debug(uint64_t excno)
{
    kpanic("debug %d",excno);
}

void exc13genprot(uint64_t excno)
{
    kpanic("General Protection Fault %d",excno);
}
