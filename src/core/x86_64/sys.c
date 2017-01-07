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
#include "../../../loader/bootboot.h"
#include "acpi.h"

/* external resources */
extern OSZ_ccb ccb;                   // CPU Control Block
extern uint32_t fg;
extern char poweroffprefix[];
extern char poweroffsuffix[];
extern uint64_t pt;
extern OSZ_rela *relas;
extern phy_t pdpe;
extern uint64_t isr_ticks[];
extern uint64_t isr_lastfps;
extern uint64_t isr_currfps;
extern uint64_t freq;
extern uint64_t hpet_addr;
extern uint64_t lapic_addr;

extern void kprintf_center(int w, int h);
extern void isr_initirq();
extern void acpi_init();
extern void acpi_early(ACPI_Header *hdr);
extern void acpi_poweroff();
extern void pci_init();

/* device drivers loaded into "system" address space */
uint64_t __attribute__ ((section (".data"))) *drivers;
uint64_t __attribute__ ((section (".data"))) *drvptr;
char __attribute__ ((section (".data"))) *drvnames;
char __attribute__ ((section (".data"))) *drvs;
char __attribute__ ((section (".data"))) *drvs_end;
uint64_t __attribute__ ((section (".data"))) *safestack;
phy_t __attribute__ ((section (".data"))) screen[2];
sysinfo_t __attribute__ ((section (".data"))) *sysinfostruc;
char __attribute__ ((section (".data"))) fn[256];

/* turn off computer */
void sys_disable()
{
    //Poweroff platform
    acpi_poweroff();
    // if it didn't work, show a message and freeze.
    kprintf_init();
    kprintf(poweroffprefix);
    fg = 0x29283f;
    kprintf_center(21, -8);
    kprintf(poweroffsuffix);
    __asm__ __volatile__ ( "1: cli; hlt; jmp 1b" : : : );
}

/* switch to system task and start executing it */
__inline__ void sys_enable()
{
    OSZ_tcb *tcb = (OSZ_tcb*)0;
    OSZ_tcb *systcb = (OSZ_tcb*)(&tmpmap);

#if DEBUG
    // initialize debugger, it can be used only with thread mappings
    dbg_init();
#endif

    // map "SYS" task's TCB
    kmap((uint64_t)&tmpmap, (uint64_t)(services[-SRV_SYS]*__PAGESIZE), PG_CORE_NOCACHE);

    // fake an interrupt handler return to force first task switch
    __asm__ __volatile__ (
        // switch to system task's address space
        "mov %0, %%rax; mov %%rax, %%cr3;"
        // clear ABI arguments
        "xorq %%rdi, %%rdi;xorq %%rsi, %%rsi;xorq %%rdx, %%rdx;xorq %%rcx, %%rcx;"
        // "return" to the thread
        "movq %1, %%rsp; movq %2, %%rbp; xchg %%bx, %%bx; iretq" :
        :
        "r"(systcb->memroot), "b"(&tcb->rip), "i"(TEXT_ADDRESS) :
        "%rsp" );
}

/* get system timestamp from a BCD date */
uint64_t sys_getts(char *p)
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
    r += h*60*60 + (bootboot.timezone + i)*60 + s;
    // we don't honor leap sec here, but this timestamp should
    // be overwritten by SYS_stime with a more accurate value
    return r;
}

/* Initialize the "SYS" task, a very special process in many ways:
 *  1. it's text segment is loaded from .text.user section
 *  2. there's an IRT after that
 *  3. instead of libraries, it has drivers
 *  4. lot of hardware specific initialization is executed here */
void sys_init()
{
    /* this code should go in service.c, but has platform dependent parts */
    uint64_t *paging = (uint64_t *)&tmpmap;
    uint64_t i=0, s;

    // get the physical page of .text.user section
    uint64_t elf = *((uint64_t*)kmap_getpte((uint64_t)&_usercode));
    // load driver database
    char *c, *f;
    drvs = (char *)fs_locate("etc/sys/drivers");
    drvs_end = drvs + fs_size;
    kmemcpy(&fn[0], "lib/sys/", 8);

    /*** Platform specific initialization ***/
    // CPU Control Block (TSS64 in kernel bss)
    kmap((uint64_t)&ccb, (uint64_t)pmm_alloc(), PG_CORE_NOCACHE);
    ccb.magic = OSZ_CCB_MAGICH;
    //usr stack (userspace, first page)
    ccb.ist1 = __PAGESIZE;
    //nmi stack (separate page in kernel space)
    ccb.ist2 = (uint64_t)safestack + (uint64_t)__PAGESIZE;
    //exception stack (separate page in kernel space)
    ccb.ist3 = (uint64_t)safestack + (uint64_t)__PAGESIZE-256;

    // parse MADT to get IOAPIC address
    acpi_early(NULL);
    // interrupt service routines (idt, pic, ioapic etc.)
    isr_init();

    OSZ_tcb *tcb = (OSZ_tcb*)(pmm.bss_end);
    pid_t pid = thread_new("SYS");
    sys_mapping = tcb->memroot;

    // map device driver dispatcher

//    if(service_loadelf("sbin/system") == (void*)(-1))
//        kpanic("unable to load ELF from /sbin/system");
//    for(i=0;paging[i]!=0;i++);
    paging[i++] = (elf & ~(__PAGESIZE-1)) | PG_USER_RO;
#if DEBUG
    if(debug&DBG_ELF)
        kprintf("  map .text.user %x (1 page) @0\n",elf & ~(__PAGESIZE-1));
#endif

    // allocate and map IRQ Routing Table right after
    // the user mode dispatcher code
    irq_routing_table = NULL;
    s = ((ISR_NUMIRQ * nrirqmax * sizeof(void*))+__PAGESIZE-1)/__PAGESIZE;
    // failsafe
    if(s<1)
        s=1;
#if DEBUG
    if(s>1)
        kprintf("WARNING irq_routing_table bigger than a page\n");
#endif
    // add extra space to allow dynamic expansion
    s++;
    // allocate IRT
    while(s--) {
        uint64_t t = (uint64_t)pmm_alloc();
        if(irq_routing_table == NULL) {
            // initialize IRT
            irq_routing_table = (uint64_t*)t;
            irq_routing_table[0] = nrirqmax;
        }
        // map IRT
        paging[i++] = t + PG_USER_RO;
    }

    // map libc
    service_loadso("lib/libc.so");
    // detect devices and load drivers (sharedlibs) for them
    drvptr = drivers;
    // default timer frequency
    freq = 0;
    if(drvs==NULL) {
        // should never happen!
#if DEBUG
        kprintf("WARNING missing /etc/sys/drivers\n");
#endif
        syslog_early("WARNING missing /etc/sys/drivers\n");
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
        // ...enumarate other system buses
    }
    // log loaded drivers
    s = 0;
    syslog_early("Loaded drivers");
    for(i=0;i<__PAGESIZE/8;i++) {
        if(drvptr[i]!=0 && drvptr[i] != s) {
            syslog_early(" %4x %s", TEXT_ADDRESS + i*__PAGESIZE, drvptr[i]);
            s = drvptr[i];
        }
    }
    drvptr = NULL;

    // dynamic linker
    if(service_rtlink()) {

        /*** Static fields in System Info Block (returned by a syscall) ***/
        // calculate addresses for screen buffers
        sysinfostruc->screen_ptr = (virt_t)BSS_ADDRESS + ((virt_t)__SLOTSIZE * ((virt_t)__PAGESIZE / 8));
        sysinfostruc->fb_ptr = sysinfostruc->screen_ptr +
            (((bootboot.fb_width * bootboot.fb_height * 4 + __SLOTSIZE - 1) / __SLOTSIZE) *
            __SLOTSIZE * (display>=DSP_STEREO_MONO?2:1));
        sysinfostruc->fb_width = bootboot.fb_width;
        sysinfostruc->fb_height = bootboot.fb_height;
        sysinfostruc->fb_scanline = bootboot.fb_scanline;
        sysinfostruc->quantum = quantum;
        sysinfostruc->debug = debug;
        sysinfostruc->display = display;
        sysinfostruc->rescueshell = rescueshell;
        //system tables
        sysinfostruc->systables[systable_acpi_ptr] = bootboot.acpi_ptr;
        sysinfostruc->systables[systable_smbi_ptr] = bootboot.smbi_ptr;
        sysinfostruc->systables[systable_efi_ptr]  = bootboot.efi_ptr;
        sysinfostruc->systables[systable_mp_ptr]   = bootboot.mp_ptr;
        sysinfostruc->systables[systable_apic_ptr] = lapic_addr;
        sysinfostruc->systables[systable_hpet_ptr] = hpet_addr;

        /*** Timer stuff ***/
        isr_tmrinit();
        sysinfostruc->freq = freq;

        /*** Double Screen stuff ***/
        // allocate and map screen buffer A
        virt_t bss = sysinfostruc->screen_ptr;
        phy_t fbp=(phy_t)bootboot.fb_ptr;
        i = ((bootboot.fb_width * bootboot.fb_height * 4 +
            __SLOTSIZE - 1) / __SLOTSIZE) * (display>=DSP_STEREO_MONO?2:1);
        while(i-->0) {
            vmm_mapbss((OSZ_tcb*)(pmm.bss_end),bss, (phy_t)pmm_allocslot(), __SLOTSIZE, PG_USER_RW);
            // save it for SYS_swapbuf
            if(!screen[0]) {
                screen[0]=pdpe;
            }
            bss += __SLOTSIZE;
        }
        // map framebuffer
        bss = sysinfostruc->fb_ptr;
        i = (bootboot.fb_scanline * bootboot.fb_height * 4 + __SLOTSIZE - 1) / __SLOTSIZE;
        while(i-->0) {
            vmm_mapbss((OSZ_tcb*)(pmm.bss_end),bss,fbp,__SLOTSIZE, PG_USER_DRVMEM);
            bss += __SLOTSIZE;
            fbp += __SLOTSIZE;
        }

        /*** IRQ routing stuff ***/
        // don't link other elfs against irq_routing_table
        irq_routing_table = NULL;
        // modify TCB for system task
        tcb->priority = PRI_SYS;
        //set IOPL=3 in rFlags to permit IO address space
        tcb->rflags |= (3<<12);
        //clear IF flag, interrupts will be enabled only
        //when system task tells to do so. It is important
        //to initialize device driver with IRQs masked.
        tcb->rflags &= ~(0x200);
        //start executing at the begining of the text segment
        tcb->rip = TEXT_ADDRESS + (&_init - &_usercode);

        // add to queue so that scheduler will know about this thread
        sched_add((OSZ_tcb*)(pmm.bss_end));
        // register system service (subsystem)
        services[-SRV_SYS] = pid;
        syslog_early("Service -%d \"%s\" registered as %x",-SRV_SYS,"SYS",pid);
    } else {
        kpanic("unable to start \"SYS\" task");
    }
}
