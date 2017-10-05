/*
 * osZ.h
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
 * @brief Mandatory include file for every OS/Z applications
 */

#ifndef _OS_Z_
#define _OS_Z_ 1

/* common includes */
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <sys/types.h>
#include <syscall.h>
/* only for user applications */
#ifndef _OSZ_CORE_H
# include <sys/platform.h>
# include <stdio.h>
# include <stdlib.h>
# include <stdarg.h>
# include <string.h>
# include <syslog.h>
# include <sound.h>
# include <inet.h>
# include <ui.h>
# include <init.h>
#endif

#endif /* osZ.h */
