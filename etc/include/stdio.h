/*
 * stdio.h
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
 * @brief ISO C99 Standard: 7.19 Input/output, plus every file and directory manipulation
 */

#ifndef _STDIO_H
#define _STDIO_H 1

/* Default buffer size.  */
#ifndef BUFSIZ
# define BUFSIZ 65536
#endif

#define EOF (-1)

/* Default path prefix for `tmpfile'.  */
#define P_tmpdir	"/tmp/"

/* The possibilities for the third argument to `fseek'.
   These values should not be changed.  */
#define SEEK_SET	0	/* Seek from beginning of file.  */
#define SEEK_CUR	1	/* Seek from current position.  */
#define SEEK_END	2	/* Seek from end of file.  */

/* Standard file descriptors.  */
#define STDIN_FILENO    0   /* Standard input. */
#define STDOUT_FILENO   1   /* Standard output. */
#define STDERR_FILENO   2   /* Standard error output. */

/* Values for the second argument to access.
   These may be OR'd together.  */
#define R_OK    A_READ      /* Test for read permission. */
#define W_OK    A_WRITE     /* Test for write permission. */
#define X_OK    A_EXEC      /* Test for execute permission. */
#define A_OK    A_APPEND    /* Test for append permission. */
#define D_OK    A_DELETE    /* Test for delete permission. */
#define F_OK    0           /* Test for existence.  */

/* open flags */
#define O_READ      (1<<0)  // read
#define O_WRITE     (1<<1)  // write
#define O_EXCL      (1<<2)  // exclusive access (lock)
#define O_APPEND    (1<<3)  // append only
#define O_CREAT     (1<<4)  // create if not exists
#define O_TRUNC     (1<<5)  // truncate
#define O_NONBLOCK  (1<<6)  // non blocking
#define O_ASYNC     (1<<7)  // no cache write-through
#define O_TMPFILE   (1<<8)  // delete on close
#define O_FIFO      (1<<9)  // open as named pipe
#define O_RDONLY    O_READ
#define O_WRONLY    O_WRITE
#define O_RDWR      (O_READ|O_WRITE)

#define O_AMSK      (O_READ|O_WRITE|O_APPEND|O_EXCL)

/* macros and flags for stat_t */
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

/* Print a message describing the meaning of the value of errno. */
extern void perror (char *s, ...);

/* Make PATH be the root directory (the starting point for absolute paths).
   This call is restricted to the super-user.  */
extern fid_t chroot (const char *__path);

/* Change the process's working directory to PATH.  */
extern fid_t chdir (const char *path);

/* Get the pathname of the current working directory in a malloc'd buffer */
extern char *getcwd ();

/* create a static mount point */
extern int mount(const char *dev, const char *mnt, const char *opts);

/* remove a static mount point, path can be either a device or a mount point */
extern int umount(const char *path);

/* Duplicate FD, returning a new file descriptor on the same file.  */
extern fid_t dup (fid_t stream);

/* Duplicate FD to FD2, closing FD2 and making it open on the same file.  */
extern fid_t dup2 (fid_t stream, fid_t stream2);

/* Create a temporary file and open it read/write. */
extern fid_t tmpfile (void);

/* Get file attributes for FILE in a read-only BUF.  */
extern stat_t *lstat (const char *path);

/* Get file attributes for a device returned in st_dev */
extern stat_t *dstat (fid_t fd);

/* Get file attributes for the file, device, pipe, or socket that
   file descriptor FD is open on and return them in a read-only BUF. */
extern stat_t *fstat (fid_t fd);

/* Open a file and create a new STREAM for it. */
extern fid_t fopen (const char *filename, mode_t oflag);
/* Open a file, replacing an existing STREAM with it. */
extern fid_t freopen (const char *filename, mode_t oflag, fid_t stream);
/* Close STREAM. */
extern int fclose (fid_t stream);
/* Close all streams. */
extern int fcloseall (void);
/* Seek to a certain position on STREAM. */
extern int fseek (fid_t stream, off_t off, int whence);
/* Rewind to the beginning of STREAM or opendir handle. */
extern void rewind (fid_t stream);
/* Return the current position of STREAM. */
extern fpos_t ftell (fid_t stream);
/* Clear the error and EOF indicators for STREAM.  */
extern void fclrerr (fid_t stream);
/* Return the EOF indicator for STREAM.  */
extern int feof (fid_t stream);
/* Return the error indicator for STREAM.  */
extern int ferror (fid_t stream);
/* Read chunks of generic data from STREAM. */
extern size_t fread (fid_t stream, void *ptr, size_t size);
/* Write chunks of generic data to STREAM. */
extern size_t fwrite (fid_t stream, void *ptr, size_t size);
/* Return the canonical absolute name of file NAME in a read-only BUF. */
extern char *realpath (const char *name);
/* Open a directory stream on PATH. Return STREAM on the directory */
extern fid_t opendir (const char *path);
/* Read a directory entry from directory stream.  Return a pointer to a
   read-only dirent_t BUF describing the entry, or NULL for EOF or error. */
extern dirent_t *readdir(fid_t dirstream);
/* Close the directory STREAM. Return 0 if successful, -1 if not. */
extern int closedir (fid_t dirstream);

/*** unimplemented ***/
#if 0
/* The possibilities for the third argument to `setvbuf'.  */
#define _IOFBF 0		/* Fully buffered.  */
#define _IOLBF 1		/* Line buffered.  */
#define _IONBF 2		/* No buffering.  */

/* Create a one-way communication channel (pipe).
   If successful, two file descriptors are stored in PIPEDES;
   bytes written on PIPEDES[1] can be read from PIPEDES[0].
   Returns 0 if successful, -1 if not.  */
extern int pipe (int pipedes[2]);

/* Make a link to FROM named TO.  */
extern int link (const char *__from, const char *__to);

/* Make a symbolic link to FROM named TO.  */
extern int symlink (const char *__from, const char *__to);

/* Remove the link NAME.  */
extern int unlink (const char *__name);

/* Remove the directory PATH.  */
extern int rmdir (const char *__path);

/* Make all changes done to FD actually appear on disk.  */
extern int fsync (int __fd);

/* Make all changes done to all files actually appear on disk.  */
extern void sync (void) __THROW;

/* Truncate FILE to LENGTH bytes.  */
extern int truncate (const char *__file, __off_t __length);

/* Truncate the file FD is open on to LENGTH bytes.  */
extern int ftruncate (int __fd, __off_t __length);

/* Remove file FILENAME.  */
extern int remove (char *filename);
/* Rename file OLD to NEW.  */
extern int rename (char *oldname, char *newname);
/* Generate a temporary filename.  */
extern char *tmpnam (char *s);
extern char *tempnam (char *dir, char *pfx);
/* Flush STREAM, or all streams if STREAM is NULL. */
extern int fflush (FILE *stream);
/* Create a new stream that refers to a memory buffer.  */
extern FILE *fmemopen (void *s, size_t len, char *modes);

/* Open a stream that writes into a malloc'd buffer that is expanded as
   necessary.  *BUFLOC and *SIZELOC are updated with the buffer's location
   and the number of characters written on fflush or fclose.  */
extern FILE *open_memstream (char **bufloc, size_t *sizeloc);
/* If BUF is NULL, make STREAM unbuffered.
   Else make it use buffer BUF, of size BUFSIZ.  */
extern void setbuf (FILE *stream, char *buf);
/* Make STREAM use buffering mode MODE.
   If BUF is not NULL, use N bytes of it for buffering;
   else allocate an internal buffer N bytes long.  */
extern int setvbuf (FILE *stream, char *buf, int modes, size_t n);
/* Make STREAM line-buffered.  */
extern void setlinebuf (FILE *stream);
/* Write formatted output to STREAM. */
extern int fprintf (FILE *stream, char *format, ...);
/* Write formatted output to stdout. */
extern int printf (char *format, ...);
/* Write formatted output to S.  */
extern int sprintf (char *s, char *format, ...);

/* Write formatted output to S from argument list ARG. */
extern int vfprintf (FILE *stream, char *format, va_list arg);
/* Write formatted output to stdout from argument list ARG. */
extern int vprintf (char *format, va_list arg);
/* Write formatted output to S from argument list ARG.  */
extern int vsprintf (char *s, char *format, va_list arg);
/* Maximum chars of output to write in MAXLEN.  */
extern int snprintf (char *s, size_t maxlen, char *format, ...);
extern int vsnprintf (char *s, size_t maxlen, char *format, va_list arg);
/* Read formatted input from STREAM. */
extern int fscanf (FILE *stream, char *format, ...);
/* Read formatted input from stdin. */
extern int scanf (char *format, ...);
/* Read formatted input from S.  */
extern int sscanf (char *s, char *format, ...);
/* Read formatted input from S into argument list ARG. */
extern int vfscanf (FILE *s, char *format, va_list arg);

/* Read formatted input from stdin into argument list ARG. */
extern int vscanf (char *format, va_list arg);

/* Read formatted input from S into argument list ARG.  */
extern int vsscanf (char *s, char *format, va_list arg);
/* Read a character from STREAM. */
extern int fgetc (FILE *stream);
extern int getc (FILE *stream);

/* Read a character from stdin. */
extern int getchar (void);
/* Write a character to STREAM. */
extern int fputc (int c, FILE *stream);
extern int putc (int c, FILE *stream);

/* Write a character to stdout. */
extern int putchar (int c);
/* Get a newline-terminated string of finite length from STREAM. */
extern char *fgets (char *s, int n, FILE *stream);
/* Read up to (and including) a DELIMITER from STREAM into *LINEPTR
   (and null-terminate it). *LINEPTR is a pointer returned from malloc (or
   NULL), pointing to *N characters of space.  It is realloc'd as
   necessary.  Returns the number of characters read (not including the
   null terminator), or -1 on error or EOF. */
extern ssize_t getdelim (char **lineptr, size_t *n, int delimiter, FILE *stream);

/* Like `getdelim', but reads up to a newline. */
extern ssize_t getline (char **lineptr, size_t *n, FILE *stream);
/* Write a string to STREAM. */
extern int fputs (char *s, FILE *stream);

/* Write a string, followed by a newline, to stdout. */
extern int puts (char *s);

/* Push a character back onto the input buffer of STREAM. */
extern int ungetc (int c, FILE *stream);

/* Get STREAM's position. */
extern int fgetpos (FILE *stream, fpos_t *pos);
/* Set STREAM's position. */
extern int fsetpos (FILE *stream, fpos_t *pos);
/* Return the system file descriptor for STREAM.  */
extern int fileno (FILE *stream);
/* Create a new stream connected to a pipe running the given command. */
extern FILE *popen (char *command, char *modes);

/* Close a stream opened by popen and return the status of its child. */
extern int pclose (FILE *stream);
/* Acquire ownership of STREAM.  */
extern void flockfile (FILE *stream);

/* Try to acquire ownership of STREAM but do not block if it is not
   possible.  */
extern int ftrylockfile (FILE *stream);

/* Relinquish the ownership granted for STREAM.  */
extern void funlockfile (FILE *stream);
#endif

#endif

#endif /* stdio.h */
