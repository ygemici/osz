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
#define SYS_setblock    ( 4)
#define SYS_ackblock    ( 5)
// file systems
#define SYS_mountfs     ( 6)
#define SYS_mount       ( 7)
#define SYS_umount      ( 8)
#define SYS_fsck        ( 9) //
// file operations
#define SYS_mknod       (10)
#define SYS_ioctl       (11)
#define SYS_lstat       (12)
#define SYS_dstat       (13)
#define SYS_fstat       (14)
#define SYS_fopen       (15)
#define SYS_fseek       (16)
#define SYS_ftell       (17)
#define SYS_fread       (18)
#define SYS_fwrite      (19) //
#define SYS_fflush      (20) //
#define SYS_feof        (21)
#define SYS_ferror      (22)
#define SYS_fclrerr     (23)
#define SYS_fclose      (24)
#define SYS_fcloseall   (25)
#define SYS_tmpfile     (26)
#define SYS_dup         (27)
#define SYS_dup2        (28)
#define SYS_link        (29) //
#define SYS_symlink     (30) //
#define SYS_unlink      (31) //
#define SYS_rename      (32) //
#define SYS_realpath    (33)
// directory operations
#define SYS_chroot      (34)
#define SYS_chdir       (35)
#define SYS_getcwd      (36)
#define SYS_mkdir       (37) //
#define SYS_rmdir       (38) //
#define SYS_mkunion     (39) //
#define SYS_rmunion     (40) //
#define SYS_opendir     (41)
#define SYS_readdir     (42)
#define SYS_rewind      (43)
// pipe operations
#define SYS_pipe        (44) //
#define SYS_popen       (45) //
#define SYS_pclose      (46) //
// socket operations

#if DEBUG
#define SYS_dump        (255)
#endif
