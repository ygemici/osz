/*
 * lib/libc/unistd.c
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
 * @brief Function implementations for unistd.h
 */
#include <osZ.h>
#include <sys/stat.h>

public char _osver[192];
public uint64_t _bogomips = 1000;
public uint64_t _alarmstep = 1000;
public uint64_t errn = SUCCESS;

/**
 * set error status, errno
 */
public void seterr(int e)
{
    errn = e;
}

/**
 * get error status, errno
 */
public int errno()
{
    return errn;
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
