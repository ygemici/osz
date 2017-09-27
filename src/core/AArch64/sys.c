/*
 * core/AArch64/sys.c
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

/* external resources */
#if DEBUG
extern uint8_t dbg_enabled;
#endif

/* device drivers map */
dataseg char *drvs;
dataseg char *drvs_end;
dataseg uint8_t sys_fault;
dataseg uint8_t idle_first;

/* system tables */
dataseg uint64_t systables[8];

/**
 * switch to the first user task and start executing it
 */
__inline__ void sys_enable()
{
#if DEBUG
    // enable debugger, it can be used only with task mappings
    dbg_enabled = true;
#endif
    sys_fault = false;

    // map FS task's TCB

    syslog_early("Initializing");
    // fake an interrupt handler return to force first task switch
}

/**
 * Initialize the "idle" task and device drivers
 */
void sys_init()
{
    // load driver database
    char *c, *f;
    drvs = (char *)fs_locate("sys/drivers");
    drvs_end = drvs + fs_size;

    /*** Platform specific initialization ***/

    /* create idle task */
    tcb_t *tcb = task_new("idle", PRI_IDLE);
    // modify TCB for idle task. Don't add to scheduler queue, normally it will never be scheduled
    //start executing a special function.
    tcb->rip = (uint64_t)&idle;
    idle_mapping = tcb->memroot;
    idle_first = true;

    syslog_early("Device drivers");
    /* interrupt service routines (idt, pic, ioapic etc.) */
    isr_init();

    /* detect devices and load drivers for them */
    if(drvs==NULL) {
        // should never happen!
#if DEBUG
        kprintf("WARNING missing /sys/drivers\n");
#endif
        syslog_early("WARNING missing /sys/drivers\n");
        // hardcoded legacy devices if driver list not found
    } else {
        kmemcpy(&fn[0], "sys/drv/", 8);
        // load devices which are not coverable by bus enumeration
        for(c=drvs;c<drvs_end;) {
            f = c; while(c<drvs_end && *c!=0 && *c!='\n') c++;
            // skip filesystem drivers and network stack protocols
            if(f[0]=='*' && f[1]==9 && 
                (f[2]!='f' || f[3]!='s') && 
                (f[2]!='n' || f[3]!='e' || f[4]!='t' || f[5]!='/')) {
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
    }
    syslog_early("Services");
}

/**
 *  Called when the "idle" task first scheduled
 */
void sys_ready()
{
}
