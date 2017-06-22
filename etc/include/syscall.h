/*
 * syscall.h
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
 * @brief OS/Z syscall services and low level messaging prototypes
 */

#ifndef _SYSCALL_H
#define _SYSCALL_H 1

// system services (negative pid_t)
#define SRV_CORE		 0
#define SRV_FS			-1
#define SRV_UI			-2
#define SRV_syslog		-3
#define SRV_net			-4
#define SRV_sound		-5
#define SRV_init		-6
#define SRV_video		-7

#define NUMSRV          8

// get function indeces and prototypes
#include <sys/core.h>
#include <sys/fs.h>
#include <sys/ui.h>
#include <sys/syslog.h>
#include <sys/net.h>
#include <sys/sound.h>
#include <sys/init.h>

#endif /* syscall.h */
