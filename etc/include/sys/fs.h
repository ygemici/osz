/*
 * sys/fs.h
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
 * @brief OS/Z system calls for fs service. Include with osZ.h
 */

#define SYS_mountfs     (3)
#define SYS_vfslocate   (4)
#define SYS_setblock    (5)

#define SYS_seek	(0x19)
#define SYS_dup		(0x1b)
#define SYS_write	(0x1d)
#define SYS_pipe	(0x21)
#define SYS_mknod	(0x25)
#define SYS_ioctl	(0x26)
#define SYS_stat	(0x29)
#define SYS_dup2	(0x2b)
#define SYS_read	(0x2c)

