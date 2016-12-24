/*
 * core/x86_64/sys.c
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
 * @brief System Task, IRQ event dispatcher
 */

#include "platform.h"
#include "../env.h"

extern OSZ_ccb ccb;
extern OSZ_pmm pmm;
extern uint64_t pt;
extern uint64_t *irq_dispatch_table;
extern uint64_t sys_mapping;
extern OSZ_rela *relas;
extern drv_t *drivers;
extern drv_t *drvptr;

/* this code should go in service.c, but has platform dependent parts */

/* Initialize the "system" task */
void sys_init()
{
    uint64_t *paging = (uint64_t *)&tmpmap;
    int i=0, s;
    uint64_t *safestack = kalloc(1);//allocate extra stack for ISRs

    // CPU Control Block (TSS64 in kernel bss)
    kmap((uint64_t)&ccb, (uint64_t)pmm_alloc(), PG_CORE_NOCACHE);
    ccb.magic = OSZ_CCB_MAGICH;
    //usr stack (userspace, first page)
    ccb.ist1 = __PAGESIZE;
    //nmi stack (separate page)
    ccb.ist2 = (uint64_t)safestack + (uint64_t)__PAGESIZE;

    // before we call loadelf for the first time and map tcb at bss_end
    // allocate space for dynamic linking
    relas = (OSZ_rela*)kalloc(2);
    drivers = (drv_t*)kalloc(2);

    OSZ_tcb *tcb = (OSZ_tcb*)(pmm.bss_end);
    pid_t pid = thread_new("system");
    subsystems[SRV_system] = pid;
    sys_mapping = tcb->memroot & ~(__PAGESIZE-1);

    // map device driver dispatcher
    /* TODO: map page at _usercode instead of sbin/system */
    if(service_loadelf("sbin/system") == (void*)(-1))
        kpanic("unable to load ELF from /sbin/system");
    // allocate and map irq dispatcher table
    for(i=0;paging[i]!=0;i++);
    irq_dispatch_table = NULL;
    s = ((ISR_NUMIRQ * nrirqmax * sizeof(void*))+__PAGESIZE-1)/__PAGESIZE;
    // failsafe
    if(s<1)
        s=1;
#if DEBUG
    if(s>1)
        kprintf("WARNING irq_dispatch_table bigger than a page\n");
#endif
    // add extra space to allow expansion
    s++;
    // allocate IRQ Dispatch Table
    while(s--) {
        uint64_t t = (uint64_t)pmm_alloc();
        if(irq_dispatch_table == NULL) {
            // initialize IRQ Dispatch Table
            irq_dispatch_table = (uint64_t*)t;
            irq_dispatch_table[0] = nrirqmax;
        }
        paging[i++] = t + PG_USER_RO;
    }
    // map libc
    service_loadso("lib/libc.so");
    // detect devices and load drivers (sharedlibs) for them
    drvptr = drivers;
    dev_init();
    drvptr = NULL;

    // dynamic linker
    service_rtlink();
    // don't link other elf's against irq_dispatch_table
    irq_dispatch_table = NULL;
    // modify TCB for system task, platform specific part
    tcb->priority = PRI_SYS;
    //set IOPL=3 in rFlags
    tcb->rflags |= (3<<12);
    //clear IF flag. interrupts will be enabled after system task
    //first times blocks
    tcb->rflags &= ~(0x200);

    // add to queue so that scheduler will know about this thread
    sched_add(pid);
}
