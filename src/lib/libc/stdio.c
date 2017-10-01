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

public int printf (const char *__restrict __format, ...)
{
    return 0;
}

/* Print a message describing the meaning of the value of errno. */
public void perror (char *s, ...)
{
#if DEBUG
    int errnum=errno();
    // FIXME: use fprintf(stderr
    dbg_printf("%s: %s\n", strerror(errnum), s);
#endif
}

/* Open a file and create a new STREAM for it. */
public fid_t fopen (char *filename, mode_t oflag)
{
    if(filename==NULL || filename[0]==0) {
        seterr(EINVAL);
        return -1;
    }
    msg_t *msg=mq_call(SRV_FS, SYS_fopen|MSG_PTRDATA, filename, strlen(filename)+1, oflag);
    if(errno())
        return -1;
    return msg->arg0;
}

/* Close STREAM. */
public int fclose (fid_t stream)
{
    msg_t *msg=mq_call(SRV_FS, SYS_fclose, stream);
    if(errno())
        return -1;
    return msg->arg0;
}

/* Seek to a certain position on STREAM. */
public int fseek (fid_t stream, off_t off, int whence)
{
    msg_t *msg=mq_call(SRV_FS, SYS_fseek, stream, off, whence);
    if(errno())
        return -1;
    return msg->arg0;
}

/* Return the current position of STREAM. */
public fpos_t ftell (fid_t stream)
{
    msg_t *msg=mq_call(SRV_FS, SYS_ftell, stream);
    if(errno())
        return 0;
    return msg->arg0;
}

/* Rewind to the beginning of STREAM or opendir handle. */
public void rewind (fid_t stream)
{
    mq_call(SRV_FS, SYS_rewind, stream);
}

/* Clear the error and EOF indicators for STREAM.  */
public void fclrerr (fid_t stream)
{
    mq_call(SRV_FS, SYS_fclrerr, stream);
}

/* Return the EOF indicator for STREAM.  */
public int feof (fid_t stream)
{
    msg_t *msg=mq_call(SRV_FS, SYS_feof, stream);
    if(errno())
        return 0;
    return msg->arg0;
}

/* Return the error indicator for STREAM.  */
public int ferror (fid_t stream)
{
    msg_t *msg=mq_call(SRV_FS, SYS_ferror, stream);
    if(errno())
        return 0;
    return msg->arg0;
}

