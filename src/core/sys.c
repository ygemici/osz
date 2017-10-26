/*
 * core/sys.c
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
 * @brief System routines
 */

#include "arch.h"

/* number of cores */
uint32_t numcores = 1;
/* current fps counter */
uint64_t currfps, lastfps;
/* locks, timer ticks, random seed, system tables */
uint64_t locks, ticks[4], srand[4], systables[8];
/* system fault code, idle first scheduled flag */
uint8_t sys_fault, idle_first;
/* memory mappings */
phy_t idle_mapping, core_mapping, identity_mapping;
/* pointer to tmpmap in PT */
uint64_t *kmap_tmp;
/* device drivers map */
char *drvs, *drvs_end;
/* pids of services. Negative pids are service ids and looked up in this table */
pid_t services[NUMSRV];
/* irq routing */
pid_t *irq_routing_table;
uint16_t maxirq, tmrirq;

/* external resources */
extern tcb_t *sched_get_tcb(pid_t task);
#if DEBUG
extern void dbg_init();
#endif

phy_t screen[2];
char fn[256];

/**
 * Initialize the "idle" task and device drivers
 */
void sys_init()
{
    // load driver database
    char *c, *f;
    drvs = (char *)fs_locate("sys/drivers");
    drvs_end = drvs + fs_size;
    /* detect devices and load drivers for them */
    if(drvs==NULL) {
        // should never happen!
        kpanic("Missing /sys/drivers\n");
    }
    /* create idle task */
    tcb_t *tcb = vmm_idletask();
    // Don't add to scheduler queue, normally it will never be scheduled
    kmemcpy(&tcb->owner, "system", 7);
    idle_mapping = tcb->memroot;

    /*** Platform specific initialization ***/
    syslog_early("Device drivers");
    platform_detect();
    /* interrupt service routines (idt, pic, ioapic etc.) */
    isr_init();
//kprintf("Drivers:\n%s",drvs);return;

    kmemcpy(&fn[0], "sys/drv/", 8);
    // load devices which are not coverable by bus enumeration
    for(c=drvs;c<drvs_end;) {
        f = c; while(c<drvs_end && *c!=0 && *c!='\n') c++;
        // skip filesystem drivers and network stack protocols
        if(f[0]=='*' && f[1]==9 && 
            (f[2]!='f' || f[3]!='s') && 
            (f[2]!='i' || f[3]!='n' || f[4]!='e' || f[5]!='t')) {
                f+=2;
                if(c-f<255-8) {
                    kmemcpy(&fn[8], f, c-f);
                    fn[c-f+8]=0;
                    drv_init(fn);
                }
                continue;
        }
        // failsafe
        if(c>=drvs_end || *c==0) break;
        if(*c=='\n') c++;
    }
    // enumerate system buses
    platform_enumerate();

    syslog_early("Services");
}

/**
 * switch to the first user task and start executing it
 */
inline void sys_enable()
{
    tcb_t *tcb = (tcb_t*)0;
    tcb_t *fstcb = (tcb_t*)(&tmpmap);

#if DEBUG
    // enable debugger, it can be used only with task mappings
    dbg_init();
#endif
    sys_fault = false;
    idle_first = true;

    // map FS task's TCB
    kmap((uint64_t)&tmpmap,
        (uint64_t)(services[-SRV_FS]*__PAGESIZE),
        PG_CORE_NOCACHE);

    syslog_early("Initializing");
    // fake an interrupt/exception handler return to force first task switch
    vmm_enable(fstcb->memroot, &tcb->rip, TEXT_ADDRESS);
}

/**
 * Add a task into IRQ Routing Table
 */
int sys_installirq(uint16_t irq, phy_t memroot)
{
    if(irq>=0 && irq<maxirq) {
        if(irq_routing_table[irq]!=0) {
            syslog_early("too many IRQ handlers for %d\n", irq);
            return EBUSY;
        }
#if DEBUG
    if(debug&DBG_IRQ)
        kprintf("IRQ %d: %x\n", irq, memroot);
#endif
        irq_routing_table[irq] = memroot;
        return SUCCESS;
    } else {
        return EINVAL;
    }
}

/**
 * Called when the "idle" task first scheduled 
 */
void sys_ready()
{
    /* finish up ISR initialization */
    int i;
    idle_first = false;
    syslog_early("IRQ Routing Table (%d IRQs)", maxirq);
    for(i=0;i<maxirq;i++) {
        if(i==tmrirq)
            syslog_early(" %3d: core (Timer)", i);
        else if(irq_routing_table[i])
            syslog_early(" %3d: %x", i, irq_routing_table[i]);
        // enable IRQs
        if(i==tmrirq || irq_routing_table[i])
            isr_enableirq(i);
    }
    /* log we're ready */
    syslog_early("Ready. Memory %d of %d pages free.", pmm.freepages, pmm.totalpages);

    // dump log even if compiled without debug support
    if(debug&DBG_LOG) {
        kprintf(syslog_buf);
        // force a pause
        scry = 65536;
        kprintf_scrollscr();
    }

    /* reset early kernel console */
    kprintf_init();

    /* Display ready message */
    kprintf("OS/Z ready. Allocated %d pages out of %d",
        pmm.totalpages - pmm.freepages, pmm.totalpages);
    kprintf(", free %d.%d%%\n",
        pmm.freepages*100/(pmm.totalpages+1), (pmm.freepages*1000/(pmm.totalpages+1))%10);
#if DEBUG
    // breakpoint for bochs
    breakpoint;
#endif

    /* disable scroll pause */
    scry = -1;
return;
    /* mount filesystems. It have to come from somewhere, so we choose the init task. */
    tcb_t *tcb=sched_get_tcb(services[-SRV_init]);
    sched_awake(tcb);
    vmm_map(tcb->memroot);
    msg_sends(EVT_DEST(SRV_FS) | EVT_FUNC(SYS_mountfs),0,0,0,0,0,0);
}
