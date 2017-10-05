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
#define SYS_ioctl       (10)
#define SYS_lstat       (11)
#define SYS_dstat       (12)
#define SYS_fstat       (13)
#define SYS_fopen       (14)
#define SYS_fseek       (15)
#define SYS_ftell       (16)
#define SYS_fread       (17)
#define SYS_fwrite      (18)
#define SYS_fflush      (19)
#define SYS_feof        (20)
#define SYS_ferror      (21)
#define SYS_fclrerr     (22)
#define SYS_fclose      (23)
#define SYS_fcloseall   (24)
#define SYS_tmpfile     (25)
#define SYS_dup         (26)
#define SYS_dup2        (27)
#define SYS_link        (28)
#define SYS_symlink     (29)
#define SYS_unlink      (30)
#define SYS_rename      (31)
#define SYS_realpath    (32)
// pipe operations
#define SYS_pipe        (33)
#define SYS_popen       (34)
#define SYS_pclose      (35)
// directory operations
#define SYS_chroot      (36)
#define SYS_chdir       (37)
#define SYS_getcwd      (38)
#define SYS_mkdir       (39)
#define SYS_rmdir       (40)
#define SYS_mkunion     (41)
#define SYS_rmunion     (42)
#define SYS_opendir     (43)
#define SYS_readdir     (44)
#define SYS_rewind      (45)
// socket operations

