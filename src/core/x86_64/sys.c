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
 * @brief System routines
 */

#include "../../../loader/bootboot.h"
#include "../env.h"
#include "acpi.h"
#include "pci.h"

/* external resources */
extern OSZ_ccb ccb;                   // CPU Control Block
extern sysinfo_t sysinfostruc;
extern uint32_t fg;
extern char rebootprefix[];
extern char poweroffprefix[];
extern char poweroffsuffix[];
extern uint64_t pt;
extern OSZ_rela *relas;
extern phy_t pdpe;

extern void kprintf_center(int w, int h);
extern void acpi_init();
extern void acpi_poweroff();
extern void pci_init();
extern void idle();
#if DEBUG
extern char *syslog_buf;
extern void dbg_putchar(int c);
#endif

/* device drivers map */
char __attribute__ ((section (".data"))) *drvs;
char __attribute__ ((section (".data"))) *drvs_end;
phy_t __attribute__ ((section (".data"))) screen[2];
char __attribute__ ((section (".data"))) fn[256];
uint8_t __attribute__ ((section (".data"))) sys_fault;
uint8_t __attribute__ ((section (".data"))) idle_first;
pid_t __attribute__ ((section (".data"))) identity_pid;

/**
 *  reboot computer
 */
void sys_reset()
{
    // say we're rebooting (on serial too)
    kprintf_init();
#if DEBUG
    dbg_putchar(13);
    dbg_putchar(10);
#endif
    kprintf(rebootprefix);
    // reboot computer
    __asm__ __volatile__ ("movb $0xFE, %%al; outb %%al, $0x64" : : : );
    // if it didn't work, show a message and freeze. Should never happen.
    fg = 0x29283f;
    kprintf_center(20, -8);
    kprintf(poweroffsuffix);
#if DEBUG
    dbg_putchar(13);
    dbg_putchar(10);
#endif
    __asm__ __volatile__ ( "1: cli; hlt; jmp 1b" : : : );
}

/**
 * turn off computer
 */
void sys_disable()
{
    // say we're finished (on serial too)
    kprintf_init();
#if DEBUG
    dbg_putchar(13);
    dbg_putchar(10);
#endif
    kprintf(poweroffprefix);
    // Poweroff real hardware
    acpi_poweroff();
    // Bochs poweroff hack
    char *s = "Shutdown";
    while (*s) {
        __asm__ __volatile__ ("movw $0x8900, %%dx; outb %0, %%dx" : : "a"(*s) : );
        s++;
    }
    // if it didn't work, show a message and freeze.
    fg = 0x29283f;
    kprintf_center(20, -8);
    kprintf(poweroffsuffix);
#if DEBUG
    dbg_putchar(13);
    dbg_putchar(10);
#endif
    __asm__ __volatile__ ( "1: cli; hlt; jmp 1b" : : : );
}

/**
 * switch to the first user task and start executing it
 */
__inline__ void sys_enable()
{
    OSZ_tcb *tcb = (OSZ_tcb*)0;
    OSZ_tcb *firsttcb = (OSZ_tcb*)(&tmpmap);

#if DEBUG
    // initialize debugger, it can be used only with thread mappings
    dbg_init();
#endif
    sys_fault = false;

    // map first device driver or if none, FS task's TCB
    kmap((uint64_t)&tmpmap,
        (uint64_t)((ccb.hd_active[PRI_DRV]!=0?ccb.hd_active[PRI_DRV]:services[-SRV_FS])*__PAGESIZE),
        PG_CORE_NOCACHE);

    // fake an interrupt handler return to force first task switch
    __asm__ __volatile__ (
        // switch to first task's address space
        "mov %0, %%rax; mov %%rax, %%cr3;"
        // clear ABI arguments
        "xorq %%rdi, %%rdi;xorq %%rsi, %%rsi;xorq %%rdx, %%rdx;xorq %%rcx, %%rcx;"
        // "return" to the thread
        "movq %1, %%rsp; movq %2, %%rbp;\n#if DEBUG\nxchg %%bx, %%bx;\n#endif\n iretq" :
        :
        "r"(firsttcb->memroot), "b"(&tcb->rip), "i"(TEXT_ADDRESS) :
        "%rsp" );
}

/**
 * Initialize the "idle" task and device drivers
 */
void sys_init()
{
    // load driver database
    char *c, *f;
    drvs = (char *)fs_locate("etc/sys/drivers");
    drvs_end = drvs + fs_size;
    kmemcpy(&fn[0], "lib/sys/", 8);

    /*** Platform specific initialization ***/
    syslog_early("Device drivers");

    /* create idle thread */
    OSZ_tcb *tcb = (OSZ_tcb*)(pmm.bss_end);
    thread_new("idle");
    // modify TCB for idle task. Don't add to queue, normally it never scheduled
    tcb->priority = PRI_IDLE;
    //start executing at the begining of the text segment.
    tcb->rip = (uint64_t)&idle;
    tcb->cs = 0x8;  //ring 0 selectors
    tcb->ss = 0x10;
    idle_mapping = tcb->memroot;
    idle_first = true;
    identity_pid = 0;

    /* interrupt service routines (idt, pic, ioapic etc.) */
    isr_init();

    /* detect devices and load drivers for them */
    if(drvs==NULL) {
        // should never happen!
#if DEBUG
        kprintf("WARNING missing /etc/sys/drivers\n");
#endif
        syslog_early("WARNING missing /etc/sys/drivers\n");
        // hardcoded legacy devices if driver list not found
        drv_init("lib/sys/input/ps2.so");
        drv_init("lib/sys/display/fb.so");
    } else {
        // load devices which don't have entry in any ACPI tables
        for(c=drvs;c<drvs_end;) {
            f = c; while(c<drvs_end && *c!=0 && *c!='\n') c++;
            // skip filesystem drivers
            if(f[0]=='*' && f[1]==9 && (f[2]!='f' || f[3]!='s')) {
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
/*
        // parse ACPI
        acpi_init();
        // enumerate PCI BUS
        pci_init();
        // ...enumarate other system buses
*/
    }
}

/*** Called when the "idle" task first scheduled ***/
void sys_ready()
{
    /* reset early kernel console */
    kprintf_reset();

    // TODO: move this printf to init
    kprintf("OS/Z ready. Allocated %d pages out of %d",
        pmm.totalpages - pmm.freepages, pmm.totalpages);
    kprintf(", free %d.%d%%\n",
        pmm.freepages*100/(pmm.totalpages+1), (pmm.freepages*1000/(pmm.totalpages+1))%10);

#if DEBUG
    if(sysinfostruc.debug&DBG_LOG)
        kprintf(syslog_buf);
#endif
    /* disable scroll pause */
    scry = -1;

#if DEBUG
    __asm__ __volatile__("xchg %bx,%bx");
#endif
    /* finish up ISR initialization */
    isr_fini();
    /* log we're ready */
    syslog_early("Ready. Memory %d of %d pages free.", pmm.freepages, pmm.totalpages);

    /* set up user services */
    if(identity_pid) {
        /* start first time turn on's set up task */
        syslog_early("Identity process started");
        sched_awake((OSZ_tcb*)identity_pid);
        /* we will notify init when identity exited, see isr_syscall in syscall.c */
    } else {
        /* notify init service to start */
        msg_sends(EVT_DEST(SRV_init) | EVT_FUNC(SYS_ack),0,0,0,0,0,0);
    }
}
