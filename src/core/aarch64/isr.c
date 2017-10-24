/*
 * core/aarch64/isr.c
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

#include "arch.h"
#include <platform.h>
#include "isr.h"

#define TMRMAX  1000000 //max 1 MHz, 1/TMRMAX = 1 usec (micro)

/* compile time check */
c_assert(ISR_NUMIRQ < 224);

/* external resources */
extern uint64_t env_getts(char *p, int16_t timezone);

/* alarm queue stuff */
uint64_t tmrfreq;
uint64_t qdiv;
uint64_t fpsdiv;
uint64_t alarmstep;
// counters
uint64_t seccnt;
uint64_t qcnt;
/* next task to schedule */
extern uint64_t sched_next;

/* irq routing */
extern pid_t *irq_routing_table;
extern uint16_t maxirq, tmrirq;

uint64_t bogomips;

/* timer ticks, random seed */
extern uint64_t ticks[4], srand[4];
extern uint32_t numcores;


/**
 * enable an IRQ
 */
void isr_enableirq(uint16_t irq)
{
}

/**
 * disable an IRQ
 */
void isr_disableirq(uint16_t irq)
{
}

/**
 * Initialize interrupts
 */
void isr_init()
{
    char *tmrname[] = { "arm", "cpu" };
    uint64_t i=0, s;
    sched_next = 0;

    /*** CPU Control Block ***/
    kmap((uint64_t)&ccb, (uint64_t)pmm_alloc(1), PG_CORE_NOCACHE|PG_PAGE);
    ccb.magic = OSZ_CCB_MAGICH;

    /*** IRQ Routing Table ***/
    s = ((ISR_NUMIRQ * sizeof(void*))+__PAGESIZE-1)/__PAGESIZE;
    // failsafe
    if(s<1)
        s=1;
    // allocate IRT
    irq_routing_table = kalloc(s);

    /*** Timer stuff ***/
    // default timer frequency and IRQ
    tmrfreq = 0; tmrirq = 0;
    // load timer driver
    platform_timer();

    /* checks */
    if(tmrfreq<1000 || tmrfreq>TMRMAX)
        kpanic("unable to load timer driver %s", tmrname[clocksource-1]);

    if(quantum<10)
        quantum=10;         //min 10 task switch per sec
    if(quantum>tmrfreq/10)
        quantum=tmrfreq/10; //max 1 switch per 10 interrupts
    // calculate stepping in microsec
    alarmstep = 1000000/tmrfreq;
    // failsafes
    if(alarmstep<1)     alarmstep=1;     //unit to add to nanosec per interrupt
    qdiv = tmrfreq/quantum; //number of ints to a task switch

    /* use bootboot.datetime and bootboot.timezone to calculate */
    ticks[TICKS_TS] = env_getts((char *)&bootboot.datetime, bootboot.timezone);
    ticks[TICKS_NTS] = ticks[TICKS_HI] = ticks[TICKS_LO] = 0;
    // set up system counters
    seccnt = 1;
    qcnt = quantum;

    // mask all IRQs
    maxirq = ISR_NUMIRQ;

    // calibrate with timer to get bus freq
    i=10000;

    /* override the default IRQ handler ISR with selected Timer ISR */

    /* add entropy */
    srand[i%4] *= 16807;
    srand[bogomips%4] *= 16807;

    i *= tmrfreq;
    bogomips *= tmrfreq;
    syslog_early(" cpu %d core(s), %d cps, %d bmips", numcores, i, bogomips);
    /* Hz: number of interrupts per sec
     * step: microsec to add to TICKS_NTS every interrupt
     * ts: task switch after every n interrupts */
    syslog_early(" timer/%s at %d Hz, step %d usec, ts %d",
        tmrname[clocksource-1], tmrfreq, alarmstep, qdiv);
}

/**
 * fallback exception handler
 */
void excabort(uint64_t excno, uint64_t rip, uint64_t rsp)
{
    kpanic("---- exception %d errcode %d ----",excno,((tcb_t*)0)->excerr);
}

/*** exception specific code ***/

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
