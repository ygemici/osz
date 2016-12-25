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

/* external resources */
extern OSZ_ccb ccb;                   // CPU Control Block

/* from isrs.S */
extern void isr_exc00divzero();
extern void isr_irq0();
extern void isr_inithw(uint64_t *idt, OSZ_ccb *tss);

extern uint64_t core_mapping;

extern uint64_t ioapic_addr;

/* preemption counter */
uint64_t __attribute__ ((section (".data"))) isr_ticks[2];
/* 256 bit random seed */
uint64_t __attribute__ ((section (".data"))) isr_entropy[4];

/* System call dispatcher for messages sent to SRV_core */
bool_t isr_syscall(pid_t thread, uint64_t event, void *ptr, size_t size, uint64_t magic)
{
    OSZ_tcb *tcb = (OSZ_tcb*)0;
    switch(MSG_FUNC(event)) {
        case SYS_exit:
            break;
        default:
            tcb->errno = EINVAL;
            return false;
    }
    return true;
}


/* Initialize interrupts */
void isr_init()
{
    uint64_t *idt = kalloc(1);      //allocate Interrupt Descriptor Table
    void *ptr;
    int i;

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

    // set up isr_syscall dispatcher and IDTR, also enable IRQs
    isr_inithw(idt, &ccb);
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
