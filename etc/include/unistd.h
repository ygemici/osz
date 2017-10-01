/*
 * unistd.h
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
 * @brief POSIX Standard: 2.10 Symbolic Constants <unistd.h>
 */

#ifndef _UNISTD_H
#define _UNISTD_H   1

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

#ifndef _AS
#include <stdio.h>

/* Return only when the bit is set and was clear, yield otherwise */
void lockacquire(int bit, uint64_t *ptr);
/* Clear a bit */
void lockrelease(int bit, uint64_t *ptr);

/* Dynamically link a symbol */
extern void *dl(uchar *sym, uchar *elf);

/* Give up CPU time */
extern void yield();

/* Make the process sleep for SEC seconds, or until a signal arrives
   and is not ignored.  The function returns the number of seconds less
   than SEC which it actually slept (thus zero if it slept the full time).
   If a signal handler does a `longjmp' or modifies the handling of the
   SIGALRM signal while inside `sleep' call, the handling of the SIGALRM
   signal afterwards is undefined.  There is no return value to indicate
   error, but if `sleep' returns SECONDS, it probably didn't work. */
extern void sleep(uint64_t sec);

/* Sleep USEC microseconds, or until a signal arrives that is not blocked
   or ignored. */
extern void usleep(uint64_t usec);

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

/* Write LENGTH bytes of randomness starting at BUFFER.  Return 0 on
   success or -1 on error.  */
int getentropy (void *buffer, size_t length);

/*** unimplemented ***/
#if 0

/* Test for access to NAME using the real UID and real GID.  */
extern int access (const char *name, int type);

/* Create a one-way communication channel (pipe).
   If successful, two file descriptors are stored in PIPEDES;
   bytes written on PIPEDES[1] can be read from PIPEDES[0].
   Returns 0 if successful, -1 if not.  */
extern int pipe (int pipedes[2]);

/* Schedule an alarm.  In SECONDS seconds, the process will get a SIGALRM.
   If SECONDS is zero, any currently scheduled alarm will be cancelled.
   The function returns the number of seconds remaining until the last
   alarm scheduled would have signaled, or zero if there wasn't one.
   There is no return value to indicate an error, but you can set `errno'
   to 0 and check its value after calling `alarm', and this might tell you.
   The signal may come late due to processor scheduling.  */
extern unsigned int alarm (uint64_t sec);
/* Set an alarm to go off (generating a SIGALRM signal) in VALUE
   microseconds.  If INTERVAL is nonzero, when the alarm goes off, the
   timer is reset to go off every INTERVAL microseconds thereafter.
   Returns the number of microseconds remaining before the alarm.  */
extern void ualarm (__useconds_t __value, __useconds_t __interval);

/* Suspend the process until a signal arrives.
   This always returns -1 and sets `errno' to EINTR.  */
extern int pause (void);

/* Duplicate FD, returning a new file descriptor on the same file.  */
extern int dup (int __fd);

/* Duplicate FD to FD2, closing FD2 and making it open on the same file.  */
extern int dup2 (int __fd, int __fd2);

/* NULL-terminated array of "NAME=VALUE" environment variables.  */
extern char **environ;

/* Replace the current process, executing PATH with arguments ARGV and
   environment ENVP.  ARGV and ENVP are terminated by NULL pointers.  */
extern int execve (const char *__path, char *const __argv[], char *const __envp[]);

/* Execute PATH with arguments ARGV and environment from `environ'.  */
extern int execv (const char *__path, char *const __argv[]);

/* Execute PATH with all arguments after PATH until a NULL pointer,
   and the argument after that for environment.  */
extern int execle (const char *__path, const char *__arg, ...);

/* Execute PATH with all arguments after PATH until
   a NULL pointer and environment from `environ'.  */
extern int execl (const char *__path, const char *__arg, ...);

/* Execute FILE, searching in the `PATH' environment variable if it contains
   no slashes, with arguments ARGV and environment from `environ'.  */
extern int execvp (const char *__file, char *const __argv[]);

/* Execute FILE, searching in the `PATH' environment variable if
   it contains no slashes, with all arguments after FILE until a
   NULL pointer and environment from `environ'.  */
extern int execlp (const char *__file, const char *__arg, ...);

/* Get the process ID of the calling process.  */
extern pid_t getpid (void);

/* Get the process ID of the calling process's parent.  */
extern pid_t getppid (void);

/* Get the real user ID of the calling process.  */
extern uid_t getuid (void);

/* If SIZE is zero, return the number of supplementary groups
   the calling process is in.  Otherwise, fill in the group IDs
   of its supplementary groups in LIST and return the number written.  */
extern size_t getgroups (size_t size, gid_t list[]);

/* Return nonzero if the calling process is in group GID.  */
extern int group_member (gid_t gid);

/* Set the user ID of the calling process to UID.
   If the calling process is the super-user, set the real
   and effective user IDs, and the saved set-user-ID to UID;
   if not, the effective user ID is set to UID.  */
extern int setuid (uid_t __uid);

/* Set the group ID of the calling process to GID.
   If the calling process is the super-user, set the real
   and effective group IDs, and the saved set-group-ID to GID;
   if not, the effective group ID is set to GID.  */
extern int addgid (gid_t __gid);
extern int delgid (gid_t __gid);

/* Clone the calling process, creating an exact copy.
   Return -1 for errors, 0 to the new process,
   and the process ID of the new process to the old process.  */
extern pid_t fork (void);

/* Clone the calling process, but without copying the whole address space.
   The calling process is suspended until the new process exits or is
   replaced by a call to `execve'.  Return -1 for errors, 0 to the new process,
   and the process ID of the new process to the old process.  */
extern pid_t vfork (void);

/* Return the pathname of the terminal FD is open on, or NULL on errors.
   The returned storage is good only until the next call to this function.  */
extern char *ttyname (int __fd);

/* Store at most BUFLEN characters of the pathname of the terminal FD is
   open on in BUF.  Return 0 on success, otherwise an error number.  */
extern int ttyname_r (int __fd, char *__buf, size_t __buflen);

/* Return 1 if FD is a valid descriptor associated
   with a terminal, zero if not.  */
extern int isatty (int __fd);

/* Make a link to FROM named TO.  */
extern int link (const char *__from, const char *__to);

/* Make a symbolic link to FROM named TO.  */
extern int symlink (const char *__from, const char *__to);

/* Read the contents of the symbolic link PATH into no more than
   LEN bytes of BUF.  The contents are not null-terminated.
   Returns the number of characters read, or -1 for errors.  */
extern ssize_t readlink (const char *__restrict __path, char *__restrict __buf, size_t __len);

/* Remove the link NAME.  */
extern int unlink (const char *__name);

/* Remove the directory PATH.  */
extern int rmdir (const char *__path);

/* Return the foreground process group ID of FD.  */
extern __pid_t tcgetpgrp (int __fd) __THROW;

/* Set the foreground process group ID of FD set PGRP_ID.  */
extern int tcsetpgrp (int __fd, __pid_t __pgrp_id) __THROW;

/* Return the login name of the user.  */
extern char *getlogin (void);

/* Put the name of the current host in no more than LEN bytes of NAME.
   The result is null-terminated if LEN is large enough for the full
   name and the terminator.  */
extern int gethostname (char *__name, size_t __len);

/* Set the name of the current host to NAME, which is LEN bytes long.
   This call is restricted to the super-user.  */
extern int sethostname (const char *__name, size_t __len);

/* Revoke access permissions to all processes currently communicating
   with the control terminal, and then send a SIGHUP signal to the process
   group of the control terminal.  */
extern int vhangup (void);

/* Revoke the access of all descriptors currently open on FILE.  */
extern int revoke (const char *__file);

/* Make all changes done to FD actually appear on disk.  */
extern int fsync (int __fd);

/* Make all changes done to all files actually appear on disk.  */
extern void sync (void) __THROW;

/* Truncate FILE to LENGTH bytes.  */
extern int truncate (const char *__file, __off_t __length);

/* Truncate the file FD is open on to LENGTH bytes.  */
extern int ftruncate (int __fd, __off_t __length);

/* Encrypt at most 8 characters from KEY using salt to perturb DES.  */
extern char *crypt (const char *__key, const char *__salt);

/* Encrypt data in BLOCK in place if EDFLAG is zero; otherwise decrypt
   block in place.  */
extern void encrypt (char *__glibc_block, int __edflag);

#endif
#endif

#endif /* unistd.h  */
