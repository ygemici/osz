/*
 * core/service.c
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
 * @brief System service loader and ELF parser
 */

#include <elf.h>
#include <sys/sysinfo.h>
#include "env.h"

/* external resources */
extern phy_t screen[2];
extern phy_t pdpe;
extern uint64_t *syslog_buf;
extern sysinfo_t sysinfostruc;
extern pid_t identity_pid;

/* pids of services. Negative pids are service ids and looked up in this */
pid_t  __attribute__ ((section (".data"))) *services;
uint64_t __attribute__ ((section (".data"))) nrservices = -SRV_USRFIRST;

/* needs screen pointer mapping */
uint8_t __attribute__ ((section (".data"))) scrptr = 0;

/**
 * register a user mode service for pid translation
 */
uint64_t service_register(pid_t thread)
{
    services[nrservices++] = thread;
    return -(nrservices-1);
}

/**
 * Initialize a non-special system service
 */
void service_init(int subsystem, char *fn)
{
    char *cmd = fn;
    while(cmd[0]!='/')
        cmd++;
    cmd++;
    pid_t pid = thread_new(cmd);
    //set priority for the task
    if(subsystem == SRV_USRFIRST) {
        ((OSZ_tcb*)(pmm.bss_end))->priority = PRI_APP;    // identity process
        identity_pid = pid;
    } else if(subsystem == SRV_USRFIRST-1)
        ((OSZ_tcb*)(pmm.bss_end))->priority = PRI_IDLE;   // screen saver
    else
        ((OSZ_tcb*)(pmm.bss_end))->priority = PRI_SRV;    // everything else
    // map executable
    if(elf_load(fn) == (void*)(-1)) {
        syslog_early("WARNING unable to load ELF from %s", fn);
        return;
    }
    // map libc and other libraries
    elf_neededso(0);

    // dynamic linker
    if(elf_rtlink()) {
        // add to queue so that scheduler will know about this thread
        sched_add((OSZ_tcb*)(pmm.bss_end));

        // block identity process at once
        if(identity_pid == pid) {
            sched_block((OSZ_tcb*)(pmm.bss_end));
        }

        if(subsystem > SRV_USRFIRST) {
            services[-subsystem] = pid;
            syslog_early("Service -%d \"%s\" registered as %x",-subsystem,cmd,pid);
        }
    } else {
        syslog_early("WARNING thread check failed for %s", fn);
    }
}

/**
 * Initialize the file system service, the "fs" task
 */
void fs_init()
{
    char *s, *f, *drvs = (char *)fs_locate("etc/sys/drivers");
    char *drvs_end = drvs + fs_size;
    char fn[256];
    pid_t pid = thread_new("FS");
    ((OSZ_tcb*)(pmm.bss_end))->priority = PRI_SRV;
    // map file system dispatcher
    if(elf_load("sbin/fs") == (void*)(-1)) {
        kpanic("unable to load ELF from /sbin/fs");
    }
    // map libc
    elf_loadso("lib/libc.so");

    // load file system drivers
    if(drvs==NULL) {
        // hardcoded if driver list not found
        // should not happen!
        syslog_early("/etc/sys/drivers not found");
        elf_loadso("lib/sys/fs/gpt.so");    // disk
        elf_loadso("lib/sys/fs/fsz.so");    // initrd and OS/Z partitions
        elf_loadso("lib/sys/fs/vfat.so");   // EFI boot partition
    } else {
        for(s=drvs;s<drvs_end;) {
            f = s; while(s<drvs_end && *s!=0 && *s!='\n') s++;
            if(f[0]=='*' && f[1]==9 && f[2]=='f' && f[3]=='s') {
                f+=2;
                if(s-f<255-8) {
                    kmemcpy(&fn[0], "lib/sys/", 8);
                    kmemcpy(&fn[8], f, s-f);
                    fn[s-f+8]=0;
                    elf_loadso(fn);
                }
                continue;
            }
            // failsafe
            if(s>=drvs_end || *s==0) break;
            if(*s=='\n') s++;
        }
    }
    elf_neededso(1);

    // dynamic linker
    if(elf_rtlink()) {
        // map the first page in bss
        vmm_mapbss((OSZ_tcb*)(pmm.bss_end), BSS_ADDRESS, (phy_t)pmm_alloc(), __PAGESIZE, PG_USER_RW);

        // map initrd in "fs" task's memory
        vmm_mapbss((OSZ_tcb*)(pmm.bss_end),BUF_ADDRESS,bootboot.initrd_ptr, bootboot.initrd_size, PG_USER_RW);

#ifdef DEBUG
        //set IOPL=3 in rFlags to permit IO address space for dbg_printf()
        ((OSZ_tcb*)(pmm.bss_end))->rflags |= (3<<12);
#endif
        // add to queue so that scheduler will know about this thread
        sched_add((OSZ_tcb*)(pmm.bss_end));

        services[-SRV_FS] = pid;
        syslog_early("Service -%d \"%s\" registered as %x",-SRV_FS,"FS",pid);
    } else {
        kpanic("thread check failed for /sbin/fs");
    }
}

/**
 * Initialize the system logger service, the "syslog" task
 */
void syslog_init()
{
    int i=0,j=0;
    uint64_t *paging = (uint64_t *)&tmpmap;
    pid_t pid = thread_new("syslog");
    ((OSZ_tcb*)(pmm.bss_end))->priority = PRI_SRV;
    // map file system dispatcher
    if(elf_load("sbin/syslog") == (void*)(-1)) {
        syslog_early("unable to load ELF from /sbin/syslog");
    }
    // map early syslog buffer after ELF
    for(i=0;paging[i]!=0;i++);
    for(j=0;j<nrlogmax;j++)
        paging[i+j] = ((*((uint64_t*)kmap_getpte(
            (uint64_t)syslog_buf + j*__PAGESIZE)))
            & ~(__PAGESIZE-1)) + PG_USER_RO;

    // map libc and other libraries
    elf_neededso(0);

    // dynamic linker
    if(elf_rtlink()) {
        // add to queue so that scheduler will know about this thread
        sched_add((OSZ_tcb*)(pmm.bss_end));

        services[-SRV_syslog] = pid;
        syslog_early("Service -%d \"%s\" registered as %x",-SRV_syslog,"syslog",pid);
    } else {
        kprintf("WARNING thread check failed for /sbin/syslog\n");
    }
}

/**
 * Initialize the user interface service, the "ui" task
 */
void ui_init()
{
    pid_t pid = thread_new("UI");
    ((OSZ_tcb*)(pmm.bss_end))->priority = PRI_SRV;
    int i;

    // map user interface code
    if(elf_load("sbin/ui") == (void*)(-1)) {
        kpanic("unable to load ELF from /sbin/ui");
    }
    // map window decoratorator
//    elf_loadso("lib/ui/decor.so");
    // map libc and other libraries
    elf_neededso(0);

    // dynamic linker
    if(elf_rtlink()) {
#ifdef DEBUG
        //set IOPL=3 in rFlags to permit IO address space for dbg_printf()
        ((OSZ_tcb*)(pmm.bss_end))->rflags |= (3<<12);
#endif
        // add to queue so that scheduler will know about this thread
        sched_add((OSZ_tcb*)(pmm.bss_end));

        services[-SRV_UI] = pid;
        syslog_early("Service -%d \"%s\" registered as %x",-SRV_UI,"UI",pid);

        // allocate and map screen buffer A
        virt_t bss=BUF_ADDRESS;
        for(i = ((bootboot.fb_width * bootboot.fb_height * 4 +
            __SLOTSIZE - 1) / __SLOTSIZE) * (display>=DSP_STEREO_MONO?2:1);i>0;i--) {
            vmm_mapbss((OSZ_tcb*)(pmm.bss_end),bss, (phy_t)pmm_allocslot(), __SLOTSIZE, PG_USER_RW);
            // save it for SYS_swapbuf
            if(!screen[0]) {
                screen[0]=pdpe;
            }
            bss += __SLOTSIZE;
        }
    } else {
        kpanic("thread check failed for /sbin/ui");
    }
}

/**
 * Initialize a device driver task
 */
void drv_init(char *driver)
{
    int i = kstrlen(driver);
    // get driver class and name from path
    char *drvname=driver + i;
    while(drvname>driver && *(drvname-1)!='/') drvname--;
    if(drvname>driver) {
        drvname--;
        while(drvname>driver && *(drvname-1)!='/') drvname--;
    }
    while(i>0 && driver[i]!='.') i--;
    driver[i]=0;

    // create a new thread...
    pid_t pid = thread_new(drvname);
    ((OSZ_tcb*)(pmm.bss_end))->priority = PRI_DRV;
    driver[i]='.';
    // ...start with driver event dispatcher
    if(elf_load("lib/sys/drv") == (void*)(-1)) {
        kpanic("unable to load ELF from /lib/sys/drv");
    }
    // map libc
    elf_loadso("lib/libc.so");
    // map the real driver as a shared object
    elf_loadso(driver);
    elf_neededso(1);

    // dynamic linker
    scrptr = false;
    if(elf_rtlink()) {
        //set IOPL=3 in rFlags to permit IO address space
        ((OSZ_tcb*)(pmm.bss_end))->rflags |= (3<<12);
        // add to queue so that scheduler will know about this thread
        sched_add((OSZ_tcb*)(pmm.bss_end));

        driver[i]=0;
        syslog_early(" %s %x pid %x",drvname,((OSZ_tcb*)(pmm.bss_end))->memroot,pid);
        driver[i]='.';

        //do we need to map screen and framebuffer?
        if(scrptr) {
            // allocate and map screen buffer B
            virt_t bss=BUF_ADDRESS;
            for(i = ((bootboot.fb_width * bootboot.fb_height * 4 +
                __SLOTSIZE - 1) / __SLOTSIZE) * (display>=DSP_STEREO_MONO?2:1);i>0;i--) {
                vmm_mapbss((OSZ_tcb*)(pmm.bss_end), bss, (phy_t)pmm_allocslot(), __SLOTSIZE, PG_USER_RW);
                // save it for SYS_swapbuf
                if(!screen[1]) {
                    screen[1]=pdpe;
                }
                bss += __SLOTSIZE;
            }
            // map framebuffer in next PDE
            bss=(virt_t)BUF_ADDRESS + ((virt_t)__SLOTSIZE * ((virt_t)__PAGESIZE / 8));
            phy_t fbp=(phy_t)bootboot.fb_ptr;
            for(i = (bootboot.fb_scanline * bootboot.fb_height * 4 + __SLOTSIZE - 1) / __SLOTSIZE;i>0;i--) {
                vmm_mapbss((OSZ_tcb*)(pmm.bss_end),bss,fbp,__SLOTSIZE, PG_USER_DRVMEM);
                bss += __SLOTSIZE;
                fbp += __SLOTSIZE;
            }
            services[-SRV_video] = pid;
        }
    } else {
        kprintf("WARNING thread check failed for %s\n", driver);
    }
}
