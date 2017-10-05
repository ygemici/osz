/*
 * lib/libc/stdio.c
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
 * @brief Function implementations for stdio.h
 */
#include <osZ.h>

#define DIRENTMAX 512
c_assert(DIRENTMAX>sizeof(dirent_t));
dirent_t *stdlib_direntbuf=NULL;
stat_t *stdlib_statbuf=NULL;

public int printf (const char *__restrict __format, ...)
{
    return 0;
}

/* Print a message describing the meaning of the value of errno. */
public void perror (char *s, ...)
{
#if DEBUG
    // FIXME: use fprintf(stderr,...
    dbg_printf("%s: %s\n", strerror(errno()), s);
#endif
}

/**
 * Make PATH be the root directory (the starting point for absolute paths).
 * This call is restricted to the super-user.
 */
public fid_t chroot(const char *path)
{
    if(path==NULL || path[0]==0) {
        seterr(EINVAL);
        return -1;
    }
    msg_t *msg=mq_call(SRV_FS, SYS_chroot|MSG_PTRDATA, path, strlen(path)+1);
    if(errno())
        return -1;
    return msg->arg0;
}

/**
 * Change the process's working directory to PATH.
 */
public fid_t chdir(const char *path)
{
    if(path==NULL || path[0]==0) {
        seterr(EINVAL);
        return -1;
    }
    msg_t *msg=mq_call(SRV_FS, SYS_chdir|MSG_PTRDATA, path, strlen(path)+1);
    if(errno())
        return -1;
    return msg->arg0;
}

/**
 * return the current working directory in a malloc'd buffer
 */
public char *getcwd()
{
    msg_t *msg=mq_call(SRV_FS, SYS_getcwd);
    if(errno())
        return NULL;
    return strdup(msg->ptr);
}

/**
 * create a static mount point
 */
public int mount(const char *dev, const char *mnt, const char *opts)
{
    char *path;
    int i,j,k;
    if(dev==NULL || dev[0]==0 || mnt==NULL || mnt[0]==0) {
        seterr(EINVAL);
        return -1;
    }
    i=strlen(dev); j=strlen(mnt); k=strlen(opts);
    path=(char*)malloc(i+j+k+3);
    if(path==NULL)
        return -1;
    memcpy((void*)path, (void*)dev, i+1);
    memcpy((void*)path+i+1, (void*)mnt, j+1);
    if(opts!=NULL && opts[0]!=0)
        memcpy((void*)path+i+1+j+1, (void*)opts, k+1);
    msg_t *msg=mq_call(SRV_FS, SYS_mount|MSG_PTRDATA, path, i+j+k+3);
    i=errno();
    free(path); // clears errno
    if(i) {
        seterr(i);
        return -1;
    }
    return msg->arg0;
}

/**
 * remove a static mount point, path can be either a device or a mount point
 */
public int umount(const char *path)
{
    if(path==NULL || path[0]==0) {
        seterr(EINVAL);
        return -1;
    }
    msg_t *msg=mq_call(SRV_FS, SYS_umount|MSG_PTRDATA, path, strlen(path)+1);
    if(errno())
        return -1;
    return msg->arg0;
}

/**
 * create a device link
 */
public int mknod(const char *devname, dev_t minor, mode_t mode, blksize_t size, blkcnt_t cnt)
{
    if(devname==NULL || devname[0]==0 || devname[0]=='/') {
        seterr(EINVAL);
        return -1;
    }
    msg_t *msg=mq_call(SRV_FS, SYS_mknod|MSG_PTRDATA, devname, strlen(devname)+1, minor, mode, size, cnt);
    if(errno())
        return -1;
    return msg->arg0;
}

/**
 * duplicate FD, returning a new file descriptor on the same file.
 */
public fid_t dup (fid_t stream)
{
    msg_t *msg=mq_call(SRV_FS, SYS_dup, stream);
    if(errno())
        return -1;
    return msg->arg0;
}

/**
 * duplicate FD to FD2, closing FD2 and making it open on the same file.
 */
public fid_t dup2 (fid_t stream, fid_t stream2)
{
    msg_t *msg=mq_call(SRV_FS, SYS_dup2, stream, stream2);
    if(errno())
        return -1;
    return msg->arg0;
}

/**
 * send an I/O control command to a device opened by fopen. BUFF can be NULL.
 */
public int ioctl(fid_t stream, uint64_t code, void *buff, size_t size)
{
    // we send it to FS, but the ack will came from a driver
    msg_t *msg=mq_call(SRV_FS, SYS_ioctl|(buff!=NULL&&size>0?MSG_PTRDATA:0), buff, size, stream, code);
    if(errno())
        return -1;
    return msg->arg0;
}

/**
 * Create a temporary file and open it read/write.
 */
public fid_t tmpfile (void)
{
    msg_t *msg=mq_call(SRV_FS, SYS_tmpfile);
    if(errno())
        return -1;
    return msg->arg0;
}

/**
 * Open a file and create a new STREAM for it.
 */
public fid_t fopen (const char *filename, mode_t oflag)
{
    if(filename==NULL || filename[0]==0) {
        seterr(EINVAL);
        return -1;
    }
    msg_t *msg=mq_call(SRV_FS, SYS_fopen|MSG_PTRDATA, filename, strlen(filename)+1, oflag, -1);
    if(errno())
        return -1;
    return msg->arg0;
}

/**
 * Open a file, replacing an existing STREAM with it.
 */
public fid_t freopen (const char *filename, mode_t oflag, fid_t stream)
{
    if(filename==NULL || filename[0]==0) {
        seterr(EINVAL);
        return -1;
    }
    msg_t *msg=mq_call(SRV_FS, SYS_fopen|MSG_PTRDATA, filename, strlen(filename)+1, oflag, stream);
    if(errno())
        return -1;
    return msg->arg0;
}

/**
 * Close STREAM.
 */
public int fclose (fid_t stream)
{
    msg_t *msg=mq_call(SRV_FS, SYS_fclose, stream);
    if(errno())
        return -1;
    return msg->arg0;
}

/**
 * Close all streams.
 */
public int fcloseall (void)
{
    msg_t *msg=mq_call(SRV_FS, SYS_fcloseall);
    if(errno())
        return -1;
    return msg->arg0;
}

/**
 * Seek to a certain position on STREAM.
 */
public int fseek (fid_t stream, off_t off, int whence)
{
    msg_t *msg=mq_call(SRV_FS, SYS_fseek, stream, off, whence);
    if(errno())
        return -1;
    return msg->arg0;
}

/**
 * Return the current position of STREAM.
 */
public fpos_t ftell (fid_t stream)
{
    msg_t *msg=mq_call(SRV_FS, SYS_ftell, stream);
    if(errno())
        return 0;
    return msg->arg0;
}

/**
 * Rewind to the beginning of STREAM or opendir handle.
 */
public void rewind (fid_t stream)
{
    mq_call(SRV_FS, SYS_rewind, stream);
}

/**
 * Clear the error and EOF indicators for STREAM.
 */
public void fclrerr (fid_t stream)
{
    mq_call(SRV_FS, SYS_fclrerr, stream);
}

/**
 * Return the EOF indicator for STREAM.
 */
public int feof (fid_t stream)
{
    msg_t *msg=mq_call(SRV_FS, SYS_feof, stream);
    if(errno())
        return 0;
    return msg->arg0;
}

/**
 * Return the error indicator for STREAM.
 */
public int ferror (fid_t stream)
{
    msg_t *msg=mq_call(SRV_FS, SYS_ferror, stream);
    if(errno())
        return 0;
    return msg->arg0;
}

/**
 * Read chunks of generic data from STREAM.
 */
public size_t fread (fid_t stream, void *ptr, size_t size)
{
    if(size>=__BUFFSIZE) {
        seterr(ENOTSHM);
        return 0;
    }
    msg_t *msg=mq_call(SRV_FS, SYS_fread, ptr, size, stream);
    if(errno())
        return 0;
    return msg->arg0;
}

/**
 * Write chunks of generic data to STREAM.
 */
public size_t fwrite (fid_t stream, void *ptr, size_t size)
{
    if(size>=__BUFFSIZE && (int64_t)ptr > 0) {
        seterr(ENOTSHM);
        return 0;
    }
    msg_t *msg=mq_call(SRV_FS, SYS_fwrite|MSG_PTRDATA, ptr, size, stream);
    if(errno())
        return 0;
    return msg->arg0;
}

/**
 * Get file attributes for FILE in a read-only BUF.
 */
public stat_t *lstat (const char *path)
{
    if(path==NULL || path[0]==0) {
        seterr(EINVAL);
        return NULL;
    }
    if(stdlib_statbuf==NULL) {
        stdlib_statbuf=malloc(sizeof(stat_t));
        if(stdlib_statbuf==NULL)
            return NULL;
    }
    mq_call(SRV_FS, SYS_lstat|MSG_PTRDATA, path, strlen(path)+1, stdlib_statbuf);
    if(!errno())
        return (stat_t*)stdlib_statbuf;
    return NULL;
}

/**
 * Get file attributes for a device returned in st_dev
 */
public stat_t *dstat (fid_t fd)
{
    if(stdlib_statbuf==NULL) {
        stdlib_statbuf=malloc(sizeof(stat_t));
        if(stdlib_statbuf==NULL)
            return NULL;
    }
    mq_call(SRV_FS, SYS_dstat, fd, 0, stdlib_statbuf);
    if(!errno())
        return (stat_t*)stdlib_statbuf;
    return NULL;
}

/**
 * Get file attributes for the file, device, pipe, or socket that
 * file descriptor FD is open on and return them in a read-only BUF.
 */
public stat_t *fstat (fid_t fd)
{
    if(stdlib_statbuf==NULL) {
        stdlib_statbuf=malloc(sizeof(stat_t));
        if(stdlib_statbuf==NULL)
            return NULL;
    }
    mq_call(SRV_FS, SYS_fstat, fd, 0, stdlib_statbuf);
    if(!errno())
        return (stat_t*)stdlib_statbuf;
    return NULL;
}

/**
 *  Return the canonical absolute name of file NAME in a malloc'd BUF.
 */
char *realpath (const char *path)
{
    if(path==NULL || path[0]==0) {
        seterr(EINVAL);
        return NULL;
    }
    msg_t *msg=mq_call(SRV_FS, SYS_realpath|MSG_PTRDATA, path, strlen(path)+1);
    if(errno())
        return NULL;
    return strdup((char*)msg->ptr);
}

/**
 * Open a directory stream on PATH. Return STREAM on the directory
 */
public fid_t opendir (const char *path)
{
    if(path==NULL || path[0]==0) {
        seterr(EINVAL);
        return -1;
    }
    msg_t *msg=mq_call(SRV_FS, SYS_opendir|MSG_PTRDATA, path, strlen(path)+1);
    if(errno())
        return -1;
    return msg->arg0;
}

/**
 * Read a directory entry from directory stream.  Return a pointer to a
 * read-only dirent_t BUF describing the entry, or NULL for EOF or error.
 */
public dirent_t *readdir(fid_t dirstream)
{
    // I have designed FS/Z carefully so that directory entries never
    // cross block borders, but other file systems (like ext2) cannot
    // guarantee that. Therefore we need a user side buffer and use
    // fread() to make sure we have the entire entry in memory
    if(stdlib_direntbuf==NULL) {
        stdlib_direntbuf=malloc(DIRENTMAX);
        if(errno())
            return NULL;
    }
    if(!fread(dirstream, stdlib_direntbuf, DIRENTMAX)) 
        return NULL;
#if DEBUG
dbg_printf("%2D\n",stdlib_direntbuf);
#endif
    mq_call(SRV_FS, SYS_readdir|MSG_PTRDATA, stdlib_direntbuf, DIRENTMAX, dirstream, stdlib_direntbuf);
    if(errno())
        return NULL;
    return (dirent_t*)stdlib_direntbuf;
}

/**
 * Close the directory STREAM. Return 0 if successful, -1 if not.
 */
public int closedir (fid_t dirstream)
{
    //use the same fclose, directory streams are not special
    msg_t *msg=mq_call(SRV_FS, SYS_fclose, dirstream);
    if(errno())
        return -1;
    return msg->arg0;
}
