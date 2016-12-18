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
 *                         OS/Z Core start                            *
 **********************************************************************
*/
void main()
{
    // this is so early, we don't have initrd in fs process' bss yet.
    // so we have to rely on identity mapping to locate files

    // initialize kernel implementation of printf
    kprintf_init();
    // parse environment
    env_init();
    // initialize physical memory manager, required by new thread creation
    pmm_init();
    // interrupt service routines (idt), initialize CCB
    isr_init();

    service_init(NULL);
/*
    // start "syslog" process so others can log errors
    service_init("sbin/syslog");
    // initialize "fs" process to load files
    fs_init();
    // initialize "ui" process to handle user input / output
    service_init("sbin/ui");
    if(networking) {
        // initialize "net" process for ipv4 and ipv6
        service_init("sbin/net");
    }
    if(sound) {
        // initialize "sound" process to handle audio channels
        service_init("sbin/sound");
    }
    // finally initialize the "system" process, the last subsystem
    // detect device drivers (parse system tables and link shared objects)
*/
    sys_init();

    if(identity) {
        // start first time set up process
        service_init("sbin/identity");
    }
    // load "init" or "sh" process
//    service_init(rescueshell ? "bin/sh" : "sbin/init");
#if DEBUG
    kprintf("OS/Z ready. Allocated %d pages out of %d.\n", pmm.totalpages - pmm.freepages, pmm.totalpages);
#endif
    // enable interrupts. After the first timer IRQ the
    // scheduler will choose a thread to run and we'll never return...
    isr_enable();
    // ...and reach this code
    __asm__ __volatile__ ( "int $13;xchgw %%bx,%%bx;cli;hlt" : : : );
    kpanic("core_init: scheduler returned to main thread");
}
