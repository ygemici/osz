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
#include "../pmm.h"
#include "tcb.h"

extern uint8_t tmp2map;
extern uint64_t fullsize;

void dev_init()
{
    OSZ_tcb *tcb = (OSZ_tcb*)&tmp2map;
    pid_t pid = thread_new();
    // this is so early, we don't have initrd in fs process' bss yet.
    // so we have to rely on identity mapping to locate the files
    fullsize = 0;
    // map dispatcher
    thread_loadelf("sbin/system");
    // map libc
    thread_loadso("lib/libc.so");
    // hardcoded devices
    thread_loadso("lib/sys/input/ps2.so");
    thread_loadso("lib/sys/display/fb.so");
    // call ACPI parser

    // modify TCB for system task
    kmap((uint64_t)&tmp2map, (uint64_t)(pid*__PAGESIZE), PG_CORE_NOCACHE);
    tcb->linkmem += fullsize;
    tcb->priority = PRI_SYS;
    //TODO: set IOPL=3 in rFlags

    // add to queue so that scheduler will know about this thread
    thread_add(pid);
}
