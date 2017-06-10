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

#define TMRMAX  1000000 //max 1 MHz, 1/TMRMAX = 1 usec (micro)

/* external resources */
extern OSZ_ccb ccb;                   // CPU Control Block

extern uint64_t ioapic_addr;
extern uint64_t hpet_addr;
extern sysinfo_t sysinfostruc;

/* from isrs.S */
extern void isr_exc00divzero();
extern void isr_irq0();
extern void isr_inithw(uint64_t *idt, OSZ_ccb *tss);
extern void isr_enableirq(uint8_t irq);
extern void isr_disableirq(uint8_t irq);
extern void isr_irqtmr();
extern void isr_irqtmr_rtc();
extern void isr_irqtmrcal();
extern void isr_irqtmrcal_rtc();
extern uint64_t isr_tmrcalibrate();
extern void acpi_early(void *hdr);

/* timer drivers */
extern void hpet_init();
extern void pit_init();
extern void rtc_init();

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
/* irq routing */
pid_t __attribute__ ((section (".data"))) *irq_routing_table;
uint16_t __attribute__ ((section (".data"))) isr_maxirq;
uint64_t __attribute__ ((section (".data"))) ioapic_addr;

uint64_t __attribute__ ((section (".data"))) bogomips;

/* get system timestamp from a BCD date */
uint64_t isr_getts(char *p, int16_t timezone)
{
    uint64_t j,r=0,y,m,d,h,i,s;
    /* detect BCD and binary formats */
    if(p[0]>=0x20 && p[0]<=0x30 && p[1]<=0x99 &&    //year
       p[2]>=0x01 && p[2]<=0x12 &&                  //month
       p[3]>=0x01 && p[3]<=0x31 &&                  //day
       p[4]<=0x23 && p[5]<=0x59 && p[6]<=0x60       //hour, min, sec
    ) {
        /* decode BCD */
        y = ((p[0]>>4)*1000)+(p[0]&0x0F)*100+((p[1]>>4)*10)+(p[1]&0x0F);
        m = ((p[2]>>4)*10)+(p[2]&0x0F);
        d = ((p[3]>>4)*10)+(p[3]&0x0F);
        h = ((p[4]>>4)*10)+(p[4]&0x0F);
        i = ((p[5]>>4)*10)+(p[5]&0x0F);
        s = ((p[6]>>4)*10)+(p[6]&0x0F);
    } else {
        /* binary */
        y = (p[1]<<8)+p[0];
        m = p[2];
        d = p[3];
        h = p[4];
        i = p[5];
        s = p[6];
    }
    uint64_t md[12] = {31,0,31,30,31,30,31,31,30,31,30,31};
    /* is year leap year? then tweak February */
    md[1]=((y&3)!=0 ? 28 : ((y%100)==0 && (y%400)!=0?28:29));

    // get number of days since Epoch, cheating
    r = 16801; // precalculated number of days (1970.jan.1-2017.jan.1.)
    for(j=2016;j<y;j++)
        r += ((j&3)!=0 ? 365 : ((j%100)==0 && (j%400)!=0?365:366));
    // in this year
    for(j=1;j<m;j++)
        r += md[j-1];
    // in this month
    r += d-1;
    // convert to sec
    r *= 24*60*60;
    // add time with timezone correction to get UTC timestamp
    r += h*60*60 + (timezone + i)*60 + s;
    // we don't honor leap sec here, but this timestamp should
    // be overwritten anyway by SYS_stime with a more accurate value
    return r;
}

/* Initialize interrupts */
void isr_init()
{
    char *tmrname[] = { "hpet", "pit", "rtc" };
    void *ptr;
    uint64_t i=0, s;

    // CPU Control Block (TSS64 in kernel bss)
    kmap((uint64_t)&ccb, (uint64_t)pmm_alloc(), PG_CORE_NOCACHE);
    ccb.magic = OSZ_CCB_MAGICH;
    //usr stack (userspace, first page)
    ccb.ist1 = __PAGESIZE;
    //nmi stack (separate page in kernel space, 512 bytes)
    ccb.ist2 = (uint64_t)safestack + (uint64_t)__PAGESIZE;
    //debug stack (rest of the page)
    ccb.ist3 = (uint64_t)safestack + (uint64_t)__PAGESIZE - 512;

    // parse MADT to get IOAPIC address and HPET address
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

    // allocate and map IRQ Routing Table
    s = (((ISR_NUMIRQ+1) * sizeof(void*))+__PAGESIZE-1)/__PAGESIZE;
    // failsafe
    if(s<1)
        s=1;
    // allocate IRT
    irq_routing_table = kalloc(s);

    // default timer frequency and IRQ
    tmrfreq = 0; tmrirq = 0;

    /* load timer driver */
    //   0 = autodetect
    //   1 = HPET
    //   2 = PIT
    //   3 = RTC
#if ISR_CTRL == CTRL_PIC
    if(clocksource>3) clocksource=0;
    if(clocksource==0) {
        clocksource=hpet_addr==0?TMR_PIT:TMR_HPET;
    }
#else
    clocksource = TMR_HPET;
#endif
    //if HPET not found, fallback to PIT
    if(clocksource==TMR_HPET && hpet_addr!=0) {
        hpet_init();
#if ISR_CTRL == CTRL_PIC
    } else if(clocksource==TMR_RTC) {
        rtc_init();
    } else {
        clocksource=TMR_PIT;
        pit_init();
#endif
    }

    /* checks */
    if(tmrfreq<1000 || tmrfreq>TMRMAX)
        kpanic("unable to load timer driver %s", tmrname[clocksource-1]);

    if(sysinfostruc.quantum<10)
        sysinfostruc.quantum=10;         //min 10 task switch per sec
    if(sysinfostruc.quantum>tmrfreq/10)
        sysinfostruc.quantum=tmrfreq/10; //max 1 switch per 10 interrupts
    //calculate stepping in microsec
    alarmstep = 1000000/tmrfreq;
    //failsafes
    if(alarmstep<1)     alarmstep=1;     //unit to add to nanosec per interrupt
    qdiv = tmrfreq/sysinfostruc.quantum; //number of ints to a task switch

    /* use bootboot.datetime and bootboot.timezone to calculate */
    sysinfostruc.ticks[TICKS_TS] = isr_getts((char *)&bootboot.datetime, bootboot.timezone);
    sysinfostruc.ticks[TICKS_NTS] = isr_currfps = sysinfostruc.fps =
    sysinfostruc.ticks[TICKS_HI] = sysinfostruc.ticks[TICKS_LO] = 0;
    // set up system counters
    seccnt = 1;
    qcnt = sysinfostruc.quantum;

    // set up isr_syscall dispatcher and IDTR, also mask all IRQs
    isr_maxirq = 0;
    isr_inithw(idt, &ccb);
    if(!isr_maxirq)
        isr_maxirq=16;

    //calibrate with timer to get clock freq
    ptr = clocksource==TMR_RTC ? &isr_irqtmrcal_rtc : &isr_irqtmrcal;
    idt[(tmrirq+32)*2+0] = IDT_GATE_LO(IDT_INT, ptr);
    idt[(tmrirq+32)*2+1] = IDT_GATE_HI(ptr);
    isr_enableirq(tmrirq);
    i=isr_tmrcalibrate();
    isr_disableirq(tmrirq);

    /* override the default IRQ handler ISR with timer ISR */
    ptr = clocksource==TMR_RTC ? &isr_irqtmr_rtc : &isr_irqtmr;
    idt[(tmrirq+32)*2+0] = IDT_GATE_LO(IDT_INT, ptr);
    idt[(tmrirq+32)*2+1] = IDT_GATE_HI(ptr);

    sysinfostruc.freq = i*tmrfreq;
    bogomips *= tmrfreq;
    syslog_early(" cpu %d cps, %d bogomips",sysinfostruc.freq, bogomips);
    /* Hz: number of interrupts per sec
     * step: microsec to add to TICKS_NTS every interrupt
     * ts: task switch after every n interrupts */
    syslog_early(" timer/%s at %d Hz, step %d usec, ts %d",
        tmrname[clocksource-1], tmrfreq, alarmstep, qdiv);
}

void isr_fini()
{
    int i;
    syslog_early("IRQ Routing Table (%d IRQs)", isr_maxirq);
    for(i=0;i<isr_maxirq;i++) {
        if(i==tmrirq)
            syslog_early(" %3d: core (Timer)", i);
        else if(irq_routing_table[i])
            syslog_early(" %3d: %x", i, irq_routing_table[i]);
        // enable IRQs
        if(i==tmrirq || irq_routing_table[i])
            isr_enableirq(i);
    }
    // "soft" IRQ #1, mem2vid in display driver
    if(irq_routing_table[ISR_NUMIRQ]!=0)
        syslog_early(" m2v: %x", irq_routing_table[ISR_NUMIRQ] );

}

int isr_installirq(uint8_t irq, phy_t memroot)
{
    if(irq>=0 && irq<isr_maxirq) {
        if(irq_routing_table[irq]!=0) {
            syslog_early("too many IRQ handlers for %d\n", irq);
            return EIO;
        }
#if DEBUG
    if(sysinfostruc.debug&DBG_IRQ)
        kprintf("IRQ %d: %x\n", irq, memroot);
#endif
        irq_routing_table[irq] = memroot;
        return SUCCESS;
    } else {
        return EINVAL;
    }
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
    kpanic("Compiled without debugger");
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
