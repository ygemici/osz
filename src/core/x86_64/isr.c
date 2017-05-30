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
extern void isr_inithw(uint64_t *idt, OSZ_ccb *tss);
extern void isr_enableirq(uint8_t irq);
extern uint64_t sys_getts(char *p);
extern void hpet_init();
extern void pit_init();
extern void rtc_init();
extern void isr_irqtmr();
extern void isr_irqtmr_rtc();
extern void acpi_early(void *hdr);

extern phy_t core_mapping;
extern uint64_t ioapic_addr;
extern sysinfo_t sysinfostruc;

/* safe stack for interrupt routines */
uint64_t __attribute__ ((section (".data"))) *safestack;

/* current fps counter */
uint64_t __attribute__ ((section (".data"))) isr_currfps;
/* alarm queue stuff */
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

/* Initialize interrupts */
void isr_init()
{
    char *tmrname[] = { "HPET", "PIT", "RTC" };
    void *ptr;
    int i;

    // CPU Control Block (TSS64 in kernel bss)
    kmap((uint64_t)&ccb, (uint64_t)pmm_alloc(), PG_CORE_NOCACHE);
    ccb.magic = OSZ_CCB_MAGICH;
    //usr stack (userspace, first page)
    ccb.ist1 = __PAGESIZE;
    //nmi stack (separate page in kernel space, 512 bytes)
    ccb.ist2 = (uint64_t)safestack + (uint64_t)__PAGESIZE;
    //debug stack (rest of the page)
    ccb.ist3 = (uint64_t)safestack + (uint64_t)__PAGESIZE - 512;

    // parse MADT to get IOAPIC address
    acpi_early(NULL);

    isr_next = 0;

    idt = kalloc(1);                                 //allocate Interrupt Descriptor Table

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
    tmrfreq = 0; tmrirq = 0;

    /* load timer driver */
    //   0 = autodetect
    //   1 = HPET
    //   2 = PIT
    //   3 = RTC
    if(clocksource>3) clocksource=0;
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
    ptr = clocksource==TMR_RTC ? &isr_irqtmr_rtc : &isr_irqtmr;

    if(clocksource==TMR_HPET)
        hpet_init();
    else if(clocksource==TMR_RTC)
        rtc_init();
    else {
        pit_init();
        clocksource=TMR_PIT;
    }
    syslog_early("Timer %s (#%d) IRQ %d at %d Hz",
        tmrname[clocksource-1], clocksource, tmrirq, tmrfreq
    );

    /* checks */
    if(tmrfreq<1000 || tmrfreq>TMRMAX)
        kpanic("unable to load timer driver");

    /* override the default IRQ handler ISR with timer ISR */
    idt[(tmrirq+32)*2+0] = IDT_GATE_LO(IDT_INT, ptr);
    idt[(tmrirq+32)*2+1] = IDT_GATE_HI(ptr);

    if(sysinfostruc.quantum<10)
        sysinfostruc.quantum=10;         //min 10 task switch per sec
    if(sysinfostruc.quantum>tmrfreq/10)
        sysinfostruc.quantum=tmrfreq/10; //max 1 switch per 10 interrupts
    //calculate stepping in nanosec
    alarmstep = 1000000000/tmrfreq;
    //failsafes
    if(alarmstep<1)     alarmstep=1;     //unit to add to nanosec per interrupt
    qdiv = tmrfreq/sysinfostruc.quantum; //number of ints to a task switch

    syslog_early("Timer step %d ns, taskswitch %d/sec",
        alarmstep, qdiv
    );

    /* use bootboot.datetime and bootboot.timezone to calculate */
    sysinfostruc.ticks[TICKS_TS] = sys_getts((char *)&bootboot.datetime);
    sysinfostruc.ticks[TICKS_NTS] = isr_currfps = sysinfostruc.fps =
    sysinfostruc.ticks[TICKS_HI] = sysinfostruc.ticks[TICKS_LO] = 0;
    // set up system counters
    seccnt = 1;
    qcnt = sysinfostruc.quantum;

    isr_enableirq(tmrirq);
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
