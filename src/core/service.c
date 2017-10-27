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
 * @brief System service and device driver loader
 */

#include <arch.h>

/* external resources */
extern char *syslog_buf;
extern char *drvs;
extern char *drvs_end;
extern char *loadedelf;

/* pids of services. Negative pids are service ids and looked up in this table */
extern pid_t  services[NUMSRV];

/* needs screen pointer mapping */
uint8_t scrptr = 0;

/* location of fstab in mapped initrd */
uint64_t fstab, fstab_size;

/* pointer to mapped buffers */
virt_t buffer_ptr, mmio_ptr, env_ptr;

/**
 * Initialize a system service. Failure to load a critical task will panic
 */
void service_init(int subsystem, char *fn)
{
    char *s, *f;
    char so[64];
    char *cmd = fn;
    while(cmd[0]!='/')
        cmd++;
    cmd++;
    int l=kstrlen(cmd);
    tcb_t *tcb = vmm_newtask(cmd, PRI_SRV);
    kmemcpy(&tcb->owner, "system", 7);

    // map executable
    kmemset(loadedelf, 0, __PAGESIZE);
    if(elf_load(fn) == (void*)(-1)) {
        if(subsystem==SRV_FS||subsystem==SRV_UI)
            kpanic("WARNING unable to load ELF from /%s",fn);
        syslog_early("WARNING unable to load ELF from /%s", fn);
        return;
    }
    // little trick to support syslog buffer
    if(subsystem==SRV_syslog)
        buffer_ptr=vmm_mapbuf(syslog_buf, nrlogmax, PG_USER_RO);
    // search for fstab so that rtlink can pass it's address
    if(subsystem==SRV_FS) {
        fstab = (uint64_t)fs_locate("sys/etc/fstab");
        fstab_size=fs_size;
    }
    // load driver modules for system services (fs and inet)
    kmemcpy(&so[0], "sys/drv/", 8);
    for(s=drvs;s<drvs_end;) {
        f = s; while(s<drvs_end && *s!=0 && *s!='\n') s++;
        if(f[0]=='*' && f[1]==9 && f+2+l<drvs_end && f[2+l]=='/' && !kmemcmp(f+2,cmd,l)) {
            f+=2;
            if(s-f<255-8) {
                kmemcpy(&so[8], f, s-f);
                so[s-f+8]=0;
                elf_loadso(so);
            }
            continue;
        }
        // failsafe
        if(s>=drvs_end || *s==0) break;
        if(*s=='\n') s++;
    }

    // dynamic linker
    if(elf_rtlink()) {
        // map the first page in bss
        vmm_mapbss(tcb, BSS_ADDRESS, (phy_t)pmm_alloc(1), __PAGESIZE, PG_USER_RW);
        tcb->allocmem++;

        // map initrd in "fs" task's memory
        if(subsystem==SRV_FS)
            vmm_mapbss(tcb,BUF_ADDRESS,bootboot.initrd_ptr, bootboot.initrd_size, PG_USER_RW);

        // add to queue so that scheduler will know about this task
        sched_add(tcb);

        services[-subsystem] = tcb->pid;
        syslog_early(" %s -%d pid %x",cmd,-subsystem,tcb->pid);
    } else {
        if(subsystem==SRV_FS||subsystem==SRV_UI)
            kpanic("WARNING task check failed for /%s", fn);
        syslog_early("WARNING task check failed for /%s", fn);
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
    // create a new task...
    tcb_t *tcb = vmm_newtask(drvname, PRI_DRV);
    kmemcpy(&tcb->owner, "system", 7);
    driver[i]='.';

    // ...start with driver event dispatcher, but don't load libc yet
    kmemset(loadedelf, 0, __PAGESIZE);
    kmemcpy(loadedelf, "libc.so", 8);
    if(elf_load("sys/driver") == (void*)(-1)) {
        kpanic("unable to load ELF from /%s","sys/driver");
    }
    // map the environment so that drivers can parse it for their config
    env_ptr=vmm_mapbuf(&environment, 1, PG_USER_RO|(1UL<<PG_NX_BIT));
    kmemset(loadedelf, 0, __PAGESIZE);
    // map the real driver as a shared object
    elf_loadso(driver);
    // map mmio area if it exists on this platform
    mmio_ptr=platform_mapdrvmem(tcb);

    // dynamic linker
    scrptr = false;
    if(elf_rtlink()) {
        //do we need to map the framebuffer?
        if(scrptr) {
            if(services[-SRV_video])
                syslog_early(" %x display driver already loaded!",drvname);
            else {
                // map framebuffer
                vmm_mapbss(tcb, (virt_t)BUF_ADDRESS,
                    (phy_t)bootboot.fb_ptr,bootboot.fb_scanline * bootboot.fb_height, PG_USER_DRVMEM);
                services[-SRV_video] = tcb->pid;
                goto drvok;
            }
        } else {
drvok:      // add to queue so that scheduler will know about this task
            sched_add(tcb);
            driver[i]=0;
            syslog_early(" %s %x pid %x",drvname,tcb->memroot,tcb->pid);
            driver[i]='.';
        }
    } else {
        syslog_early("WARNING task check failed for /%s", driver);
    }
}
