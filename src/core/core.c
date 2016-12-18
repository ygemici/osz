/*
 * core/core.c
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
 * @brief Core
 *
 *   Memory map
 *       -512M framebuffer                   (0xFFFFFFFFE0000000)
 *       -2M core        bootboot struct     (0xFFFFFFFFFFE00000)
 *           -2M+1page   environment         (0xFFFFFFFFFFE01000)
 *           -2M+2page.. core text segment v (0xFFFFFFFFFFE02000)
 *           -2M+Xpage.. core bss          v
 *            ..0        core stack        ^
 *       0-16G RAM identity mapped
 */

#include "env.h"
extern OSZ_pmm pmm;

/**********************************************************************
 *                         OS/Z Life Cycle                            *
 **********************************************************************
*/
void main()
{
    /* step 0: open eyes */
    // initialize kernel implementation of printf
    kprintf_init();
    // parse environment
    env_init();
    // initialize physical memory manager, required by new thread creation
    pmm_init();
    // interrupt service routines (idt), initialize CCB
    isr_init();

    /* step 1: historic memory */
    service_init(0, NULL);
    // start "syslog" process so other subsystems can log errors
    // to solve the chicken egg scenario here, service_init()
    // does not use filesystem drivers, it has a built-in fs reader.
    service_init(SUB_SYSLOG, "sbin/syslog");
    // initialize "fs" process
    fs_init();

    /* step 2: communication */
    // initialize "ui" process to handle user input / output
    service_init(SUB_UI, "sbin/ui");
    if(networking) {
        // initialize "net" process for ipv4 and ipv6
        service_init(SUB_NET, "sbin/net");
    }
    if(sound) {
        // initialize "sound" process to handle audio channels
        service_init(SUB_SOUND, "sbin/sound");
    }

    /* step 3: motoric reflexes */
    // finally initialize the "system" process, the last subsystem
    // detect device drivers (parse system tables and load sharedlibs)
    sys_init();

    /* step 4: who am I */
    if(identity) {
        // start first time set up process
        service_init(USER_PROCESS, "sbin/identity");
    }

    /* step 5: stand up and prosper. */
    // load "init" or "sh" process
    service_init(USER_PROCESS, rescueshell ? "bin/sh" : "sbin/init");
#if DEBUG
    kprintf("OS/Z ready. Allocated %d pages out of %d.\n", pmm.totalpages - pmm.freepages, pmm.totalpages);
#endif
    kprintf_clr();
    // enable interrupts. After the first timer IRQ the
    // scheduler will choose a thread to run and we'll...
    isr_enable();

    /* step 6: go to dreamless sleep. */
    // ...should not reach this code until shutdown process finished
    dev_poweroff();

    __asm__ __volatile__ ( "int $1;xchgw %%bx,%%bx;cli;hlt" : : : );
}
