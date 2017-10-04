/*
 * sys/stat.h
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
 * @brief Structure of buffer for stat() libc call
 */

#ifndef _SYS_STAT_H
#define _SYS_STAT_H

#define S_IFLG   0xFF000000 // mask
#define S_IFLNK	 0x01000000 // fsdrv specific, symlink
#define S_IFUNI  0x02000000 // fsdrv specific, directory union
#define S_IFCHR  0x03000000 // character device, if blksize==1
#define S_IFMT     0xFF0000 // mask
#define S_IFREG    0x000000 // FCB_TYPE_REG_FILE
#define S_IFDIR    0x010000 // FCB_TYPE_REG_DIR
#define S_IFDEV    0x020000 // FCB_TYPE_DEVICE
#define S_IFIFO    0x030000 // FCB_TYPE_PIPE
#define S_IFSOCK   0x040000 // FCB_TYPE_SOCKET

#define S_ISLNK(m)	(((m) & S_IFLG) == S_IFLNK)
#define S_ISREG(m)	(((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m)	(((m) & S_IFMT) == S_IFDIR)
#define S_ISUNI(m)	(((m) & S_IFMT) == S_IFDIR && ((m) & S_IFLG) == S_IFUNI)
#define S_ISCHR(m)	(((m) & S_IFMT) == S_IFDEV && ((m) & S_IFLG) == S_IFCHR)
#define S_ISBLK(m)	(((m) & S_IFMT) == S_IFDEV && ((m) & S_IFLG) != S_IFCHR)
#define S_ISFIFO(m)	(((m) & S_IFMT) == S_IFIFO)
#define S_ISSOCK(m)	(((m) & S_IFMT) == S_IFSOCK)

#ifndef _AS
typedef struct {
    fid_t     st_dev;       // ID of device, *not* major/minor
    ino_t     st_ino;       // inode of the file
    mode_t    st_mode;      // mode
    uint8_t   st_type[4];   // file type
    uint8_t   st_mime[44];  // mime type
    nlink_t   st_nlink;     // number of hard links
    uid_t     st_owner;     // owner user id
    off_t     st_size;      // file size
    blksize_t st_blksize;   // block size
    blkcnt_t  st_blocks;    // number of allocated blocks
    time_t    st_atime;     // access time in microsec timestamp
    time_t    st_mtime;     // modification time in microsec timestamp
    time_t    st_ctime;     // status change time in microsec timestamp
} stat_t;

/* Get file attributes for FILE in a read-only BUF.  */
extern stat_t *lstat (const char *path);

/* Get file attributes for a device returned in st_dev */
extern stat_t *dstat (fid_t fd);

/* Get file attributes for the file, device, pipe, or socket that
   file descriptor FD is open on and return them in a read-only BUF. */
extern stat_t *fstat (fid_t fd);

#endif

#endif
