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
 * @brief ISO C99 Standard: 7.19 Input/output
 */

#ifndef _STDIO_H
#define _STDIO_H 1

#include <sys/types.h>

/* Default buffer size.  */
#ifndef BUFSIZ
# define BUFSIZ 65536
#endif

#define EOF (-1)

/* The possibilities for the third argument to `fseek'.
   These values should not be changed.  */
#define SEEK_SET	0	/* Seek from beginning of file.  */
#define SEEK_CUR	1	/* Seek from current position.  */
#define SEEK_END	2	/* Seek from end of file.  */
#define SEEK_DATA	3	/* Seek to next data.  */
#define SEEK_HOLE	4	/* Seek to next hole.  */

/* The possibilities for the third argument to `setvbuf'.  */
#define _IOFBF 0		/* Fully buffered.  */
#define _IOLBF 1		/* Line buffered.  */
#define _IONBF 2		/* No buffering.  */

/* Default path prefix for `tempnam' and `tmpnam'.  */
#define P_tmpdir	"/tmp"

#if 0
/* Remove file FILENAME.  */
extern int remove (char *filename);
/* Rename file OLD to NEW.  */
extern int rename (char *oldname, char *newname);
/* Create a temporary file and open it read/write.

   This function is a possible cancellation point and therefore not
   marked with __THROW.  */
extern FILE *tmpfile (void);
/* Generate a temporary filename.  */
extern char *tmpnam (char *s);
extern char *tempnam (char *dir, char *pfx);
/* Close STREAM. */
extern int fclose (FILE *stream);
/* Flush STREAM, or all streams if STREAM is NULL. */
extern int fflush (FILE *stream);
/* Close all streams. */
extern int fcloseall (void);
/* Open a file and create a new stream for it. */
extern FILE *fopen (char *filename, char *modes);
/* Open a file, replacing an existing stream with it. */
extern FILE *freopen (char *filename, char *modes, FILE *stream);
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

/* Read chunks of generic data from STREAM. */
extern size_t fread (void *ptr, size_t size, size_t n, FILE *stream);
/* Write chunks of generic data to STREAM. */
extern size_t fwrite (void *ptr, size_t size, size_t n, FILE *stream);
/* Seek to a certain position on STREAM. */
extern int fseek (FILE *stream, fpos_t off, int whence);
/* Return the current position of STREAM. */
extern fpos_t ftell (FILE *stream);
/* Rewind to the beginning of STREAM. */
extern void rewind (FILE *stream);
/* Get STREAM's position. */
extern int fgetpos (FILE *stream, fpos_t *pos);
/* Set STREAM's position. */
extern int fsetpos (FILE *stream, fpos_t *pos);
/* Clear the error and EOF indicators for STREAM.  */
extern void clearerr (FILE *stream);
/* Return the EOF indicator for STREAM.  */
extern int feof (FILE *stream);
/* Return the error indicator for STREAM.  */
extern int ferror (FILE *stream);
/* Print a message describing the meaning of the value of errno. */
extern void perror (char *s);
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

#endif /* stdio.h */
