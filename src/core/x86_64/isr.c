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


#include <fsZ.h>
#include "isr.h"

/* external resources */
extern OSZ_ccb ccb;                   // CPU Control Block

/* from isrs.S */
extern void isr_exc00divzero();
extern void isr_irq0();
extern void isr_inithw(uint64_t *idt, OSZ_ccb *tss);
extern uint64_t sys_getts(char *p);

extern phy_t core_mapping;
extern uint64_t ioapic_addr;
extern sysinfo_t sysinfostruc;

/* current fps counter */
uint64_t __attribute__ ((section (".data"))) isr_currfps;
/* alarm queue stuff */
uint64_t __attribute__ ((section (".data"))) freq;
uint64_t __attribute__ ((section (".data"))) fpsdiv;
uint64_t __attribute__ ((section (".data"))) quantumdiv;
uint64_t __attribute__ ((section (".data"))) alarmstep;
/* next task to schedule */
uint64_t __attribute__ ((section (".data"))) isr_next;

/* Initialize interrupts */
void isr_init()
{
    uint64_t *idt = kalloc(1);      //allocate Interrupt Descriptor Table
    void *ptr;
    int i;

    isr_next = 0;

    // generate IDT
    ptr = &isr_exc00divzero;
    // 0-31 exception handlers
    for(i=0;i<32;i++) {
        idt[i*2+0] = IDT_GATE_LO(i==2||i==8?IDT_NMI:(i==1||i==3?IDT_DBG:IDT_EXC), ptr);
        idt[i*2+1] = IDT_GATE_HI(ptr);
        ptr+=ISR_EXCMAX;
    }
    // 32-255 irq handlers
    ptr = &isr_irq0;
    for(i=32;i<ISR_NUMIRQ+32;i++) {
        idt[i*2+0] = IDT_GATE_LO(IDT_INT, ptr);
        idt[i*2+1] = IDT_GATE_HI(ptr);
        ptr+=ISR_IRQMAX;
    }
    // set up isr_syscall dispatcher and IDTR, also mask all IRQs
    isr_inithw(idt, &ccb);
}

/* initialize timer values */
void isr_tmrinit()
{
        /* checks */
        if(freq==0)
            kpanic("unable to load timer driver");
        if(freq<1000)       freq=1000;      //min 1000 interrupts per sec
        if(freq>1000000000) freq=1000000000;//max 1GHz
        if(sysinfostruc.quantum<10)      sysinfostruc.quantum=10;     //min 10 task switch per sec
        if(sysinfostruc.quantum>freq/4)  sysinfostruc.quantum=freq/4; //max 1 switch per 4 interrupts
        if(fps<5)           fps=5;          //min 5 frames per sec
        if(fps>freq/16)     fps=freq/16;    //max 1 mem2vid per 16 interrupts
        //calculate stepping in nanosec
        alarmstep = 1000000000/freq;
        //failsafes
        if(alarmstep<1)     alarmstep=1;    //unit to add to nanosec per interrupt
        quantumdiv = freq/sysinfostruc.quantum;          //number of ints to a task switch
        fpsdiv = freq/fps;                  //number of ints to a mem2vid
        if(quantumdiv*4 > fpsdiv)
            fpsdiv = quantumdiv/4;

        syslog_early("Timer: IRQ %d at %d Hz, step %d ns",
            ISR_IRQTMR, freq, alarmstep
        );
        syslog_early("Timer: task %d ints, frame %d ints",
            quantumdiv, fpsdiv
        );
        /* use bootboot.datetime and bootboot.timezone to calculate */
        sysinfostruc.ticks[TICKS_TS] = sys_getts((char *)&bootboot.datetime);
        sysinfostruc.ticks[TICKS_NTS] = isr_currfps = sysinfostruc.fps =
        sysinfostruc.ticks[TICKS_HI] = sysinfostruc.ticks[TICKS_LO] = 0;
        // set up system counters
        sysinfostruc.ticks[TICKS_SEC] = freq;
        sysinfostruc.ticks[TICKS_QUANTUM] = sysinfostruc.quantum;
        sysinfostruc.ticks[TICKS_FPS] = fpsdiv;
}

/* fallback exception handler */
void excabort(uint64_t excno, uint64_t rip, uint64_t rsp)
{
    kpanic("---- exception %d errcode %d ----",excno,((OSZ_tcb*)0)->excerr);
}

/* exception specific code */

void exc00divzero(uint64_t excno, uint64_t rip, uint64_t rsp)
{
    kpanic("divzero exception %d",excno);
}

void exc01debug(uint64_t excno, uint64_t rip, uint64_t rsp)
{
#if DEBUG
    dbg_enable(rip,rsp,NULL);
#else
    kpanic("debug exception %d", excno);
#endif
}

void exc03chkpoint(uint64_t excno, uint64_t rip, uint64_t rsp)
{
#if DEBUG
    dbg_enable(rip,rsp,"chkpoint");
#else
    kpanic("chkpoint exception %d", excno);
#endif
}

void exc13genprot(uint64_t excno, uint64_t rip, uint64_t rsp)
{
    kpanic("General Protection Fault %d",rip);
}
