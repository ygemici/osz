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
#include "acpi.h"

/* external resources */
extern uint32_t fg;
extern char poweroffprefix[];
extern char poweroffsuffix[];
extern OSZ_ccb ccb;
extern OSZ_pmm pmm;
extern uint64_t pt;
extern uint64_t *irq_dispatch_table;
extern uint64_t sys_mapping;
extern OSZ_rela *relas;

extern void kprintf_center(int w, int h);
extern void isr_initirq();
extern void acpi_init();
extern void acpi_early(ACPI_Header *hdr);
extern void acpi_poweroff();
extern void pci_init();

/* device drivers loaded into "system" address space */
drv_t __attribute__ ((section (".data"))) *drivers;
drv_t __attribute__ ((section (".data"))) *drvptr;

char *sys_getdriver(char *device, char *drvs, char *drvs_end)
{
    return NULL;
}

/* turn off computer */
void sys_poweroff()
{
    // APCI poweroff
    acpi_poweroff();
    // if it didn't work, show a message and freeze.
    kprintf_init();
    kprintf(poweroffprefix);
    fg = 0x29283f;
    kprintf_center(20, -8);
    kprintf(poweroffsuffix);
    __asm__ __volatile__ ( "1: cli; hlt; jmp 1b" : : : );
}

/* switch to system task and start executing it */
void sys_enable()
{
    OSZ_tcb *tcb = (OSZ_tcb*)0;
    OSZ_tcb *systcb = (OSZ_tcb*)(&tmpmap);

    // map "system" task's TCB
    kmap((uint64_t)&tmpmap, (uint64_t)(subsystems[SRV_system]*__PAGESIZE), PG_CORE_NOCACHE);

    // fake an interrupt handler return to start multitasking
    __asm__ __volatile__ (
        // switch task's address space
        "mov %0, %%rax; mov %%rax, %%cr3;"
        // clear ABI arguments
        "xorq %%rdi, %%rdi;xorq %%rsi, %%rsi;xorq %%rdx, %%rdx;xorq %%rcx, %%rcx;"
        // "return" to the thread
        "movq %1, %%rsp; movq %2, %%rbp; xchg %%bx, %%bx; iretq" :
        :
        "r"(systcb->memroot), "b"(&tcb->rip), "i"(TEXT_ADDRESS) :
        "%rsp" );
}

/* Initialize the "system" task */
void sys_init()
{
    /* this code should go in service.c, but has platform dependent parts */
    uint64_t *paging = (uint64_t *)&tmpmap;
    int i=0, s;
    uint64_t *safestack = kalloc(1);//allocate extra stack for ISRs
    char *c, *f, *drvs = (char *)fs_locate("etc/sys/drivers");
    char *drvs_end = drvs + fs_size;
    char fn[256];

    // CPU Control Block (TSS64 in kernel bss)
    kmap((uint64_t)&ccb, (uint64_t)pmm_alloc(), PG_CORE_NOCACHE);
    ccb.magic = OSZ_CCB_MAGICH;
    //usr stack (userspace, first page)
    ccb.ist1 = __PAGESIZE;
    //nmi stack (separate page)
    ccb.ist2 = (uint64_t)safestack + (uint64_t)__PAGESIZE;

    // parse MADT to get IOAPIC address
    acpi_early(NULL);
    // interrupt service routines (idt)
    isr_init();

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
    if(drvs==NULL) {
        // should never happen!
#if DEBUG
        kprintf("WARNING missing /etc/sys/drivers\n");
#endif
        // hardcoded legacy devices if driver list not found
        service_loadso("lib/sys/input/ps2.so");
        service_loadso("lib/sys/display/fb.so");
        service_loadso("lib/sys/proc/pitrtc.so");
    } else {
        // load devices which don't have entry in any ACPI tables
        for(c=drvs;c<drvs_end;) {
            f = c; while(c<drvs_end && *c!=0 && *c!='\n') c++;
            // skip filesystem drivers
            if(f[0]=='*' && f[1]==9 && (f[2]!='f' || f[3]!='s')) {
                f+=2;
                if(c-f<255-8) {
                    kmemcpy(&fn[0], "lib/sys/", 8);
                    kmemcpy(&fn[8], f, c-f);
                    fn[c-f+8]=0;
                    service_loadso(fn);
                }
                continue;
            }
            // failsafe
            if(c>=drvs_end || *c==0) break;
            if(*c=='\n') c++;
        }
        // parse ACPI
        acpi_init();
        // enumerate PCI BUS
        pci_init();
// TODO:  service_installirq(irq, ehdr->e_shoff);
    }
    drvptr = NULL;

    // dynamic linker
    service_rtlink();
    // don't link other elf's against irq_dispatch_table
    irq_dispatch_table = NULL;
    // modify TCB for system task, platform specific part
    tcb->priority = PRI_SYS;
    //set IOPL=3 in rFlags
    tcb->rflags |= (3<<12);
    //clear IF flag so that interrupts will be enabled only
    //after system task blocked for the first time. It is important
    //to initialize device driver with IRQs masked.
    tcb->rflags &= ~(0x200);

    // add to queue so that scheduler will know about this thread
    sched_add(pid);
}
