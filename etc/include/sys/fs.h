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

// cache
#define SYS_setblock    ( 3)
#define SYS_ackblock    ( 4)
// file systems
#define SYS_mountfs     ( 5)
#define SYS_mount       ( 6)
#define SYS_umount      ( 7)
#define SYS_sync        ( 8)
// file operations
#define SYS_mknod       ( 9)
#define SYS_lstat       (10)
#define SYS_dstat       (11)
#define SYS_fstat       (12)
#define SYS_fopen       (13)
#define SYS_fseek       (14)
#define SYS_ftell       (15)
#define SYS_fread       (16)
#define SYS_fwrite      (17)
#define SYS_fflush      (18)
#define SYS_feof        (19)
#define SYS_ferror      (20)
#define SYS_fclrerr     (21)
#define SYS_fclose      (22)
#define SYS_ioctl       (23)
#define SYS_dup         (24)
#define SYS_dup2        (25)
#define SYS_pipe        (26)
#define SYS_link        (27)
#define SYS_symlink     (28)
#define SYS_readlink    (29)
#define SYS_unlink      (30)
#define SYS_rename      (31)
#define SYS_realpath    (32)
// directory operations
#define SYS_chroot      (33)
#define SYS_chdir       (34)
#define SYS_getcwd      (35)
#define SYS_mkdir       (36)
#define SYS_rmdir       (37)
#define SYS_mkunion     (38)
#define SYS_rmunion     (39)
#define SYS_opendir     (40)
#define SYS_readdir     (41)
#define SYS_rewind      (42)
#define SYS_closedir    (43)

