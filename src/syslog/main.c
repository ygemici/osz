/*
 * syslog/main.c
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
 * @brief Syslog service
 */
#include <osZ.h>

// import early syslog buffer from rtlinker
extern unsigned char *_syslog_ptr;
extern uint64_t _syslog_size;

public void openlog(char *ident, int option, int facility) {}
public void closelog() {}
public void syslog(int pri, char *fmt, ...) {}
public void vsyslog(int pri, char *fmt, va_list ap) {}

void task_init(int argc, char **argv)
{
}
