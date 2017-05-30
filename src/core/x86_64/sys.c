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

#include "../../../loader/bootboot.h"
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
#if DEBUG
extern void dbg_putchar(int c);
#endif

/* device drivers loaded into "system" address space */
char __attribute__ ((section (".data"))) *drvs;
char __attribute__ ((section (".data"))) *drvs_end;
phy_t __attribute__ ((section (".data"))) screen[2];
char __attribute__ ((section (".data"))) fn[256];
uint8_t __attribute__ ((section (".data"))) sys_fault;

/* reboot computer */
void sys_reset()
{
    //say we're finished (on serial too)
    kprintf_init();
#if DEBUG
    dbg_putchar(13);
    dbg_putchar(10);
#endif
    kprintf(rebootprefix);
    // reboot computer
    __asm__ __volatile__ ("movb $0xFE, %%al; outb %%al, $0x64" : : : );
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

/* turn off computer */
void sys_disable()
{
    //say we're finished (on serial too)
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

/* switch to system task and start executing it */
__inline__ void sys_enable()
{
    OSZ_tcb *tcb = (OSZ_tcb*)0;
    OSZ_tcb *fstcb = (OSZ_tcb*)(&tmpmap);

#if DEBUG
    // initialize debugger, it can be used only with thread mappings
    dbg_init();
#endif
    sys_fault = false;

    // map first device driver or FS task's TCB
    kmap((uint64_t)&tmpmap,
        (uint64_t)((ccb.hd_active[PRI_DRV]!=0?ccb.hd_active[PRI_DRV]:services[-SRV_FS])*__PAGESIZE),
        PG_CORE_NOCACHE);

    // fake an interrupt handler return to force first task switch
    __asm__ __volatile__ (
        // switch to filesystem task's address space
        "mov %0, %%rax; mov %%rax, %%cr3;"
        // clear ABI arguments
        "xorq %%rdi, %%rdi;xorq %%rsi, %%rsi;xorq %%rdx, %%rdx;xorq %%rcx, %%rcx;"
        // "return" to the thread
        "movq %1, %%rsp; movq %2, %%rbp;\n#if DEBUG\nxchg %%bx, %%bx;\n#endif\n iretq" :
        :
        "r"(fstcb->memroot), "b"(&tcb->rip), "i"(TEXT_ADDRESS) :
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

/* Initialize the "idle" task and device drivers */
void sys_init()
{
    /* this code should go in service.c, but has platform dependent parts */
    uint64_t *paging = (uint64_t *)&tmpmap;
    uint64_t i=0;

    // get the physical page of .text.user section
    uint64_t elf = *((uint64_t*)kmap_getpte((uint64_t)&_usercode));
    // load driver database
    char *c, *f;
    drvs = (char *)fs_locate("etc/sys/drivers");
    drvs_end = drvs + fs_size;
    kmemcpy(&fn[0], "lib/sys/", 8);

    /*** Platform specific initialization ***/
    syslog_early("Device drivers");

    // interrupt service routines (idt, pic, ioapic etc.)
    isr_init();

    /* create idle thread */
    OSZ_tcb *tcb = (OSZ_tcb*)(pmm.bss_end);
    thread_new("idle");
    paging[i++] = (elf & ~(__PAGESIZE-1)) | PG_USER_RO;
#if DEBUG
    if(sysinfostruc.debug&DBG_ELF)
        kprintf("  maptext .text.user %x (1 page) @0\n",elf & ~(__PAGESIZE-1));
#endif
    // modify TCB for idle task
    tcb->priority = PRI_IDLE;
    //start executing at the begining of the text segment
    tcb->rip = TEXT_ADDRESS + (&_init - &_usercode);
    // add to queue so that scheduler will know about this thread
    sched_add((OSZ_tcb*)(pmm.bss_end));

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
        /*** Static fields in System Info Block (returned by a syscall) ***/
        // calculate addresses for screen buffers
/*
        sysinfostruc.screen_ptr = (virt_t)BSS_ADDRESS + ((virt_t)__SLOTSIZE * ((virt_t)__PAGESIZE / 8));
        sysinfostruc.fb_ptr = sysinfostruc.screen_ptr +
            (((bootboot.fb_width * bootboot.fb_height * 4 + __SLOTSIZE - 1) / __SLOTSIZE) *
            __SLOTSIZE * (sysinfostruc.display>=DSP_STEREO_MONO?2:1));
        sysinfostruc.fb_width = bootboot.fb_width;
        sysinfostruc.fb_height = bootboot.fb_height;
        sysinfostruc.fb_scanline = bootboot.fb_scanline;
        //system tables, platform specific
        sysinfostruc.systables[systable_acpi_idx] = bootboot.acpi_ptr;
        sysinfostruc.systables[systable_smbi_idx] = bootboot.smbi_ptr;
        sysinfostruc.systables[systable_efi_idx]  = bootboot.efi_ptr;
        sysinfostruc.systables[systable_mp_idx]   = bootboot.mp_ptr;
*/
        /*** Double Screen stuff ***/
        // allocate and map screen buffer A
/*
        virt_t bss = sysinfostruc.screen_ptr;
        phy_t fbp=(phy_t)bootboot.fb_ptr;
        i = ((bootboot.fb_width * bootboot.fb_height * 4 +
            __SLOTSIZE - 1) / __SLOTSIZE) * (sysinfostruc.display>=DSP_STEREO_MONO?2:1);
        while(i-->0) {
            vmm_mapbss((OSZ_tcb*)(pmm.bss_end),bss, (phy_t)pmm_allocslot(), __SLOTSIZE, PG_USER_RW);
            // save it for SYS_swapbuf
            if(!screen[0]) {
                screen[0]=pdpe;
            }
            bss += __SLOTSIZE;
        }
        // map framebuffer
        bss = sysinfostruc.fb_ptr;
        i = (bootboot.fb_scanline * bootboot.fb_height * 4 + __SLOTSIZE - 1) / __SLOTSIZE;
        while(i-->0) {
            vmm_mapbss((OSZ_tcb*)(pmm.bss_end),bss,fbp,__SLOTSIZE, PG_USER_DRVMEM);
            bss += __SLOTSIZE;
            fbp += __SLOTSIZE;
        }
*/
    // log irq routing table
    isr_fini();
}
