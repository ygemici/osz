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

#include "env.h"

#if DEBUG
extern char *syslog_buf;
#endif

/**********************************************************************
 *                         OS/Z Life Cycle                            *
 **********************************************************************
*/
void main()
{
    kprintf("OS/Z starting...\n");
    // note: we cannot call syslog_early("starting") as syslog_buf
    // is not allocated yet.

    /* step 1: motoric reflexes */
    // parse environment
    env_init();
    // initialize physical memory manager, required by new thread creation
    pmm_init();
    // this is early, we don't have "FS" subsystem yet.

    // initialize the "SYS" task, service_init(SRV_SYS, "sbin/system")
    // this will also detect device drivers
    sys_init();
    // initialize "FS" task, special service_init(SRV_FS, "sbin/fs")
    fs_init();

    /* step 2: communication */
    // initialize "UI" task to handle user input / output
    // special service_init(SRV_UI, "sbin/ui")
    ui_init();

    // other means of communication
    // start "syslog" task so others can log errors
    // special service_init(SRV_syslog, "sbin/syslog")
    syslog_init();
    if(networking) {
        // initialize "net" task for ipv4 and ipv6 routing
        service_init(SRV_net, "sbin/net");
    }
    if(sound) {
        // initialize "sound" task to handle audio channels
        service_init(SRV_sound, "sbin/sound");
    }
    // load screen saver
//    service_init(SRV_USRLAST, "sbin/scrsvr");

    /* step 3: who am I */
    fs_locate("etc/hostname");
    if(identity || fs_size==0) {
        // start first time turn on's set up task
        service_init(SRV_USRFIRST, "sbin/identity");
    }

    /* step 4: stand up and prosper. */
    // load "init" or "sh" process
    if(rescueshell)
        service_init(SRV_USRFIRST, "bin/sh");
    else
        service_init(SRV_init, "sbin/init");

    // The "ready" message. Cover out "starting" message
    syslog_early("Ready. %d of %d free.",pmm.totalpages - pmm.freepages, pmm.totalpages);
    kprintf_reset();
    kprintf("OS/Z ready. Allocated %d pages out of %d",
        pmm.totalpages - pmm.freepages, pmm.totalpages);
    kprintf(", free %d.%d%%\n",
        pmm.freepages*100/(pmm.totalpages+1), (pmm.freepages*1000/(pmm.totalpages+1))%10);
#if DEBUG
    if(debug&DBG_LOG)
        kprintf(syslog_buf);
#endif
    //disable scroll pause
    scry = -1;

    // enable system task. That will initialize devices and then blocks.
    // When that happens, scheduler will choose a task to run and...
    sys_enable();
    // ...we should never return here. Instead sched_pick() will
    // call sys_disable() when no tasks left during shutdown procedure.
    // But just in case of unwanted return, we call poweroff anyway.

    /* step 5: go to dreamless sleep. */
    sys_disable();
}
