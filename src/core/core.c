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

#include "core.h"
#include "pmm.h"

extern uint8_t identity_map;
extern uint8_t networking;
extern uint8_t rescueshell;

/**********************************************************************
 *                         OS/Z Core start                            *
 **********************************************************************
*/
void main()
{
    // this is so early, we don't have initrd in fs process' bss yet.
    // so we have to rely on identity mapping to locate files
    identity_map = true;
    // initialize kernel implementation of printf
    kprintf_init();
    // parse environment
    env_init();
    // initialize physical memory manager, required by new thread creation
    pmm_init();
    // interrupt service routines (idt), initialize CCB
    isr_init();
    // start "syslog" process so others can log errors
    service_init("sbin/syslog");
    // initialize "fs" process to load files from initrd
    service_init("sbin/fs");
    // initialize "ui" process to handle user input / output
    service_init("sbin/ui");
    if(networking) {
        // initialize "net" process for ipv4 and ipv6
        service_init("sbin/net");
    }
    // initialize the "system" process
    // detect device drivers (parse system tables and link shared objects)
    dev_init();
    // load "init" or "sh" process
    service_init(rescueshell ? "bin/sh" : "sbin/init");

    identity_map = false;

    __asm__ __volatile__ ( "int $1;xchgw %%bx,%%bx;cli;hlt" : : : );

}
