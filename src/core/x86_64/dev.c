/*
 * core/x86_64/dev.c
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
 * @brief Starts "system" process with device drivers
 */

#include "../core.h"
#include "tcb.h"

extern OSZ_pmm pmm;
extern void acpi_init();

void dev_init()
{
    // this is so early, we don't have initrd in fs process' bss yet.
    // so we have to rely on identity mapping to locate the files
    OSZ_tcb *tcb = (OSZ_tcb*)(pmm.bss_end);
    pid_t pid = thread_new();
    // map device driver dispatcher
    thread_loadelf("sbin/system");
    // map libc
    thread_loadso("lib/libc.so");
    // call ACPI parser to detect devices and load drivers for them
    acpi_init();

    // dynamic linker
    thread_dynlink(pid);
    // modify TCB for system task
    tcb->priority = PRI_SYS;
    //TODO: set IOPL=3 in rFlags

    // add to queue so that scheduler will know about this thread
    thread_add(pid);
}
