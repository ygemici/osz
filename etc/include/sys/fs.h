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

// file systems and cache
#define SYS_mountfs     ( 3)
#define SYS_mount       ( 4)
#define SYS_umount      ( 5)
#define SYS_setblock    ( 6)
#define SYS_addvolume   ( 7)
#define SYS_delvolume   ( 8)
// file operations
#define SYS_mknod       ( 9)
#define SYS_stat        (10)
#define SYS_open        (11)
#define SYS_seek        (12)
#define SYS_read        (13)
#define SYS_write       (14)
#define SYS_ioctl       (15)
#define SYS_dup         (16)
#define SYS_dup2        (17)
#define SYS_pipe        (18)
#define SYS_fstat       (19)
#define SYS_close       (20)
#define SYS_symlink     (21)
#define SYS_readlink    (22)
#define SYS_unlink      (23)
// directory operations
#define SYS_mkdir       (24)
#define SYS_rmdir       (25)
#define SYS_opendir     (26)
#define SYS_readdir     (27)
#define SYS_rewinddir   (28)
#define SYS_closedir    (29)

