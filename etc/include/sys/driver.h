/*
 * sys/driver.h
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
 * @brief OS/Z Device Driver services
 */

#ifndef _SYS_DRIVER_H
#define _SYS_DRIVER_H 1

#define DRV_IRQ         (0)
#define DRV_ack         (1)
#define DRV_nack        (2)
#define DRV_read        (3|MSG_PTRDATA)
#define DRV_write       (4|MSG_PTRDATA)
#define DRV_ioctl       (5)

/*** libc implementation prototypes */
#ifndef _AS

// import environment configuration
extern unsigned char *_environment;

typedef struct {
    uint64_t freepages;
    uint64_t totalpages;
} meminfo_t;

void setirq(int8_t irq);                // set irq message for this task
meminfo_t meminfo();                    // get memory info
size_t mapfile(void *bss, char *fn);    // map a file on initrd
extern uint64_t mknod();                // create an entry in devfs

#endif

#endif /* sys/driver.h */
