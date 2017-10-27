/*
 * core/main.c
 *
 * Copyright 2017 CC-by-nc-sa bztsrc@github
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
 *       -512G shared memory                    (0xFFFFFF8000000000)
 *         -1G initial ramdisk                  (0xFFFFFFFFC0000000)
 *       -128M MMIO on Raspberry Pi             (0xFFFFFFFFF8000000)
 *        -64M framebuffer for kprintf          (0xFFFFFFFFFC000000)
 *         -2M core      bootboot[1] struct     (0xFFFFFFFFFFE00000)
 *         -2M + 1page   environment[2]         (0xFFFFFFFFFFE01000)
 *         -2M + 2pages  core text segment      (0xFFFFFFFFFFE02000)
 *         -2M + Xpages  core bss          v    (0xFFFFFFFFFFExxxxx)
 *         ..0           core boot stack   ^    (0x0000000000000000)

 *       0-16G user      RAM identity mapped[3] (0x0000000400000000)
 *
 *   [1] see https://github.com/bztsrc/bootboot/blob/master/bootboot.h
 *   [2] see sys/config and env.c. Plain ascii key=value pairs,
 *       separated by newline characters.
 *   [3] when a new task created by task_new(), user task address space
 *       will be mapped instead into 0 - 2^48.
 */

#include <arch.h>

/* OS version string */
char osver[] =
    OSZ_NAME " " OSZ_VER " " OSZ_ARCH "-" OSZ_PLATFORM " (build " OSZ_BUILD ")\n"
    "Use is subject to license terms. Copyright 2017 bzt (bztsrc@github), CC-by-nc-sa\n\n";

/**********************************************************************
 *                         OS/Z Life Cycle                            *
 **********************************************************************/
void main()
{
    // note: we cannot call syslog_early("starting") as syslog_buf
    // is not allocated yet.
    kprintf("OS/Z starting...\n");

    /*** step 1: motoric reflexes ***/
    // parse environment
    env_init();
    // initialize Physical Memory Manager, required by new task creation and syslog
    pmm_init();
    // initialize the system, "idle" task and device driver tasks
    sys_init();
    // initialize "FS" task
    service_init(SRV_FS, "sys/fs");

    /*** step 2: communication ***/
    // initialize "UI" task to handle user input / output
//    service_init(SRV_UI, "sys/ui");

    // other means of communication
/*
    if(syslog) {
        // start "syslog" task so others can log errors
        service_init(SRV_syslog, "sys/syslog")
    }
    if(networking) {
        // initialize "inet" task for ipv4 and ipv6 routing
        service_init(SRV_inet, "sys/inet");
    }
    if(sound) {
        // initialize "sound" task to handle audio channels
        service_init(SRV_sound, "sys/sound");
    }
*/

kprintf("So far so good!\n");platform_waitkey();platform_reset();
    /*** step 3: stand up and prosper. ***/
/*
#if DEBUG
    service_init(SRV_init, debug&DBG_TESTS? "sys/bin/test" : "sys/init");
#else
    service_init(SRV_init, "sys/init");
#endif
    // enable system multitasking. That will start by iterating
    // through device driver's initialization routines. Each in different
    // task, scheduler will choose them one by one and...
    sys_enable();
*/
    /*** step 4: go to dreamless sleep. ***/
    // ...we should never return here. Instead exiting init will poweroff
    // the machine as the last step in the shutdown procedure. But just in
    // case of any unwanted return, we say good bye here too.
    kprintf_poweroff();
}
