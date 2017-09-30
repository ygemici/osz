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
#define SYS_fopen       (11)
#define SYS_fseek       (12)
#define SYS_ftell       (13)
#define SYS_fread       (14)
#define SYS_fwrite      (15)
#define SYS_ioctl       (16)
#define SYS_dup         (17)
#define SYS_dup2        (18)
#define SYS_pipe        (19)
#define SYS_fstat       (20)
#define SYS_feof        (21)
#define SYS_ferror      (22)
#define SYS_fclrerr     (23)
#define SYS_fclose      (24)
#define SYS_union       (25)
#define SYS_symlink     (26)
#define SYS_readlink    (27)
#define SYS_unlink      (28)
#define SYS_rename      (29)
// directory operations
#define SYS_chroot      (30)
#define SYS_chdir       (31)
#define SYS_getcwd      (32)
#define SYS_mkdir       (33)
#define SYS_rmdir       (34)
#define SYS_opendir     (35)
#define SYS_readdir     (36)
#define SYS_rewind      (37)
#define SYS_closedir    (38)

