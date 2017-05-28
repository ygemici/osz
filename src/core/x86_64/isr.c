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

#define TMRMAX  1000000000 //max 1 GHz

/* external resources */
extern OSZ_ccb ccb;                   // CPU Control Block

/* from isrs.S */
extern void isr_exc00divzero();
extern void isr_irq0();
extern void isr_irqtmr_simple();
extern void isr_irqtmr_extra();
extern void isr_inithw(uint64_t *idt, OSZ_ccb *tss);
extern uint64_t sys_getts(char *p);

extern phy_t core_mapping;
extern uint64_t ioapic_addr;
extern sysinfo_t sysinfostruc;

/* current fps counter */
uint64_t __attribute__ ((section (".data"))) isr_currfps;
/* alarm queue stuff */
uint64_t __attribute__ ((section (".data"))) tmrisr;
uint64_t __attribute__ ((section (".data"))) tmrfreq;
uint8_t  __attribute__ ((section (".data"))) tmrirq;
uint64_t __attribute__ ((section (".data"))) qdiv;
uint64_t __attribute__ ((section (".data"))) fpsdiv;
uint64_t __attribute__ ((section (".data"))) alarmstep;
// counters
uint64_t __attribute__ ((section (".data"))) seccnt;
uint64_t __attribute__ ((section (".data"))) qcnt;
/* next task to schedule */
uint64_t __attribute__ ((section (".data"))) isr_next;

/* allocated memory */
uint64_t __attribute__ ((section (".data"))) *idt;
uint64_t __attribute__ ((section (".data"))) *isrmem;

/* Initialize interrupts */
void isr_init()
{
    void *ptr;
    int i;

    isr_next = 0;

    idt = kalloc(1);                                 //allocate Interrupt Descriptor Table
    isrmem = pmm.bss_end; pmm.bss_end += __PAGESIZE; //"allocate" space for isr mapping

    // generate Interrupt Descriptor Table
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

    // default timer frequency and IRQ
    tmrfreq = tmrisr = 0; tmrirq = 0;
}

void isr_clocksource()
{
    /* load timer driver */
    //   0 = autodetect
    //   1 = HPET
    //   2 = PIT
    //   3 = RTC
    //   4 = Local APIC timer
    //   5 = Local x2APIC timer
    if(clocksource>5) clocksource=0;
    if(clocksource==0) {
        clocksource=sysinfostruc.systables[systable_hpet_idx]==0?
        (ISR_CTRL==CTRL_PIC?2:
        (ISR_CTRL==CTRL_APIC?4:
            5)):
        1;
#if DEBUG
        if(sysinfostruc.debug&DBG_DEVICES)
            kprintf("Autodetected clocksource %d\n",clocksource);
#endif
    }
    //if HPET or LAPIC not found, fallback to PIT
/*
    if((clocksource==1 && sysinfostruc.systables[systable_hpet_idx]==0) ||
       (clocksource==4 && sysinfostruc.systables[systable_apic_idx]==0))
        clocksource=2;
*/
    service_loadso(
        clocksource==1?"lib/sys/proc/hpet.so":(
        clocksource==2?"lib/sys/proc/pit.so":(
        clocksource==3?"lib/sys/proc/rtc.so":(
        clocksource==4?"lib/sys/proc/apic.so":
        "lib/sys/proc/x2apic.so"
    ))));
}

/* initialize timer values */
void isr_tmrinit()
{
    /* checks */
    if(tmrfreq<1000 || tmrfreq>TMRMAX)
        kpanic("unable to load timer driver");

    if(tmrisr) {
        /* relocate timer isr to core space, because we cannot switch to SYS task
           normally this does not needed at all, save RTC */
        kmap((uint64_t)isrmem,(uint64_t)(tmrisr&~(__PAGESIZE-1)),PG_CORE);
        tmrisr = (uint64_t)isrmem + (tmrisr&(__PAGESIZE-1));
        /* override the default IRQ handler ISR with timer ISR */
        idt[(tmrirq+32)*2+0] = IDT_GATE_LO(IDT_INT, &isr_irqtmr_extra);
        idt[(tmrirq+32)*2+1] = IDT_GATE_HI(&isr_irqtmr_extra);
    } else {
        /* override the default IRQ handler ISR with timer ISR */
        idt[(tmrirq+32)*2+0] = IDT_GATE_LO(IDT_INT, &isr_irqtmr_simple);
        idt[(tmrirq+32)*2+1] = IDT_GATE_HI(&isr_irqtmr_simple);
    }

    if(sysinfostruc.quantum<10)
        sysinfostruc.quantum=10;         //min 10 task switch per sec
    if(sysinfostruc.quantum>tmrfreq/10)
        sysinfostruc.quantum=tmrfreq/10; //max 1 switch per 10 interrupts
    //calculate stepping in nanosec
    alarmstep = 1000000000/tmrfreq;
    //failsafes
    if(alarmstep<1)     alarmstep=1;     //unit to add to nanosec per interrupt
    qdiv = tmrfreq/sysinfostruc.quantum; //number of ints to a task switch

    syslog_early("Timer IRQ %d at %d Hz, step %d ns, tsw %d ints",
        tmrirq, tmrfreq, alarmstep, qdiv
    );

    /* use bootboot.datetime and bootboot.timezone to calculate */
    sysinfostruc.ticks[TICKS_TS] = sys_getts((char *)&bootboot.datetime);
    sysinfostruc.ticks[TICKS_NTS] = isr_currfps = sysinfostruc.fps =
    sysinfostruc.ticks[TICKS_HI] = sysinfostruc.ticks[TICKS_LO] = 0;
    // set up system counters
    seccnt = 1;
    qcnt = sysinfostruc.quantum;
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
