/*
 * core/main.c
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
 * @brief Core, boot environment
 *
 *   Memory map
 *       -512M framebuffer                      (0xFFFFFFFFE0000000)
 *       -4M tmpmq       message queue [1]      (0xFFFFFFFFFFC00000)
 *       -2M core        bootboot[2] struct     (0xFFFFFFFFFFE00000)
 *         -2M + 1page   environment[3]         (0xFFFFFFFFFFE01000)
 *         -2M + 2page.. core text segment v    (0xFFFFFFFFFFE02000)
 *         -2M + Xpage.. core bss          v    (0xFFFFFFFFFFExxxxx)
 *         ..0           core boot stack   ^    (0x0000000000000000)

 *       0-16G user      RAM identity mapped[4] (0x0000000000000000)
 *
 *   [1] see msg_t in etc/include/sys/types.h
 *   [2] see loader/bootboot.h
 *   [3] see etc/CONFIG and env.h. Plain ascii key=value pairs,
 *       separated by whitespace characters. Filled up with spaces
 *       to page size.
 *   [4] when main() calls isr_enable(), user thread will be mapped
 *       instead into 0 - 2^56 and shared memory in -2^56 - -512M.
 */

#include <lastbuild.h>
#include "env.h"

char __attribute__ ((section (".data"))) osver[] = OSZ_NAME " " OSZ_VER "-" ARCH " (build " OSZ_BUILD ")\n"
    "Use is subject to license terms. CC-by-nc-sa, copyright bzt (bztsrc@github)\n\n";

/**********************************************************************
 *                         OS/Z Life Cycle                            *
 **********************************************************************
*/
void main()
{
    // note: we cannot call syslog_early("starting") as syslog_buf
    // is not allocated yet.
    kprintf("OS/Z starting...\n");

    /*** step 1: motoric reflexes ***/
    // parse environment
    env_init();
    // initialize physical memory manager, required by new thread creation
    pmm_init();

    // initialize the system, "idle" task and device driver tasks
    sys_init();
    // initialize "FS" task, special service_init(SRV_FS, "sbin/fs")
    fs_init();

    /*** step 2: communication ***/
    // initialize "UI" task to handle user input / output
    // special service_init(SRV_UI, "sbin/ui")
    ui_init();

    // other means of communication
/*
    if(syslog) {
        // start "syslog" task so others can log errors
        // special service_init(SRV_syslog, "sbin/syslog")
        syslog_init();
    }
    if(networking) {
        // initialize "net" task for ipv4 and ipv6 routing
        service_init(SRV_net, "sbin/net");
    }
    if(sound) {
        // initialize "sound" task to handle audio channels
        service_init(SRV_sound, "sbin/sound");
    }
*/
    // load screen saver
//    service_init(SRV_USRFISRT-1, "sbin/scrsvr");

    /*** step 3: stand up and prosper. ***/
#if DEBUG
    service_init(SRV_init, debug&DBG_TESTS? "bin/test" : "sbin/init");
#else
    service_init(SRV_init, "sbin/init");
#endif
    if(identity) {
        /* start first time turn on's set up task */
        service_init(SRV_USRFIRST, "sbin/identity");
    }
    // enable system multitasking. That will start by iterating
    // through device driver's initialization routines. Each in different
    // task, scheduler will choose them one by one and...
    sys_enable();

    /*** step 4: go to dreamless sleep. ***/
    // ...we should never return here. Instead syscall will call sys_disable()
    // as the last step in the shutdown procedure when "init" task exits.
    // But just in case of any unwanted return, we call poweroff anyway.
    sys_disable();
}
