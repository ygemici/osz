/*
 * stdlib.h
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
 * @brief ISO C99 Standard: 7.20 General utilities, plus OS/Z specific functions
 */

#ifndef _STDLIB_H
#define _STDLIB_H 1

/* We define these the same for all machines.
   Changes from this to the outside world should be done in `_exit'.  */
#define EXIT_FAILURE    1   /* Failing exit status.  */
#define EXIT_SUCCESS    0   /* Successful exit status.  */
#include <sysexits.h>       /* For services */

/* Maximum length of a multibyte character in the current locale.  */
#define MB_CUR_MAX  4

/* Sharing type flags (must choose one and only one of these). */
#define MAP_SHARED  0x01        /* Share changes.  */
#define MAP_PRIVATE 0x02        /* Changes are private.  */

/* Other flags.  */
#define MAP_FIXED       0x10        /* Interpret addr exactly.  */
#define MAP_FILE        0
#define MAP_ANONYMOUS   0x20        /* Don't use a file.  */
#define MAP_SPARE       0x40        /* Don't alloc physical page */
#define MAP_ANON        MAP_ANONYMOUS
#define MAP_LOCKED      0x100       /* Lock the mapping.  */
#define MAP_NONBLOCK    0x200       /* Do not block on IO.  */

/* Flags to `msync'.  */
#define MS_ASYNC        1       /* Sync memory asynchronously.  */
#define MS_SYNC         4       /* Synchronous memory sync.  */
#define MS_INVALIDATE   2       /* Invalidate the caches.  */

/* Return value of `mmap' in case of an error.  */
#define MAP_FAILED  ((void *) -1)

#ifdef DEBUG
/* debug flags, and user mode kprintf */
#include <sys/debug.h>
#ifndef _AS
extern uint32_t _debug;
extern void dbg_printf(char * fmt, ...);
#endif
#endif

#ifndef _AS
extern char _osver[];

/* memory manager */
/* Map addresses starting near ADDR and extending for LEN bytes.
   The return value is the actual mapping address chosen or MAP_FAILED
   for errors (in which case `errno' is set).  A successful `mmap' call
   deallocates any previous mapping for the affected region.  */
extern void *mmap (void *addr, size_t len, int flags);

/* Deallocate any mapping for the region starting at ADDR and extending LEN
   bytes.  Returns 0 if successful, -1 for errors (and sets errno).  */
extern int munmap (void *addr, size_t len);

/* Guarantee all whole pages mapped by the range [ADDR,ADDR+LEN) to
   be memory resident.  */
extern int mlock (const void *addr, size_t len);

/* Unlock whole pages previously mapped by the range [ADDR,ADDR+LEN).  */
extern int munlock (const void *addr, size_t len);

/* memory allocator. Use different macros if you want a different allocator */
extern void *bzt_alloc(uint64_t *arena,size_t a,void *ptr,size_t s,int flag);
extern void bzt_free(uint64_t *arena, void *ptr);
extern void bzt_dumpmem(uint64_t *arena);

/* Task Local Storage */
/* Allocate SIZE bytes of memory.  */
#define malloc(s) bzt_alloc((void*)BSS_ADDRESS,8,NULL,s,MAP_PRIVATE)
/* Allocate NMEMB elements of SIZE bytes each, all initialized to 0.  */
#define calloc(n,s) bzt_alloc((void*)BSS_ADDRESS,8,NULL,n*s,MAP_PRIVATE)
/* Re-allocate the previously allocated block
   in PTR, making the new block SIZE bytes long.  */
#define realloc(p,s) bzt_alloc((void*)BSS_ADDRESS,8,p,s,MAP_PRIVATE)
/* ISO C variant of aligned allocation.  */
#define aligned_alloc(a,s) bzt_alloc((void*)BSS_ADDRESS,a,NULL,s,MAP_PRIVATE)
/* Free a block allocated by `malloc', `realloc' or `calloc'.  */
#define free(p) bzt_free((void*)BSS_ADDRESS,p)

/* Shared Memory */
#define smalloc(s) bzt_alloc((void*)SBSS_ADDRESS,8,NULL,s,MAP_SHARED)
#define scalloc(n,s) bzt_alloc((void*)SBSS_ADDRESS,8,NULL,n*s,MAP_SHARED)
#define srealloc(p,s) bzt_alloc((void*)SBSS_ADDRESS,8,p,s,MAP_SHARED)
#define saligned_alloc(s) bzt_alloc((void*)SBSS_ADDRESS,a,NULL,s,MAP_SHARED)
#define sfree(p) bzt_free((void*)SBSS_ADDRESS,p)

/* Return the absolute value of X.  */
#define abs(x) ((x)<0?-(x):(x))
#define labs(x) ((x)<0?-(x):(x))
#define llabs(x) ((x)<0?-(x):(x))
/* Minimum and maximum. Not in POSIX standard, but this is the right place */
#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

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

/* Write LENGTH bytes of randomness starting at BUFFER.  Return 0 on
   success or -1 on error.  */
int getentropy (void *buffer, size_t length);

/* Return a random integer between 0 and RAND_MAX inclusive.  */
extern uint64_t rand (void);
/* Seed the random number generator with the given number.  */
extern void srand (uint64_t seed);

/* Register a function to be called when `exit' is called.  */
extern int atexit (void (*func) (void));

/* Call all functions registered with `atexit' and `on_exit',
   in the reverse of the order in which they were registered,
   perform stdio cleanup, and terminate program execution with STATUS.  */
extern void exit (int status) __attribute__ ((__noreturn__));

/* Abort execution and generate a core-dump.  */
extern void abort (void)  __attribute__ ((__noreturn__));

/* Return the length of the multibyte character
   in S, which is no longer than N.  */
extern int mblen (char *s, size_t n);

/* Do a binary search for KEY in BASE, which consists of NMEMB elements
   of SIZE bytes each, using CMP to perform the comparisons.  */
extern void *bsearch (void *key, void *base, size_t nmemb, size_t size, int (*cmp)(void *, void *));

/* Sort NMEMB elements of BASE, of SIZE bytes each,
   using CMP to perform the comparisons.  */
extern void qsort (void *base, size_t nmemb, size_t size, int (*cmp)(void *, void *));

extern unsigned char *stdlib_dec(unsigned char *s, uint64_t *v, uint64_t min, uint64_t max);
extern unsigned char *stdlib_hex(unsigned char *s, uint64_t *v, uint64_t min, uint64_t max);

/* Return the `div_t', `ldiv_t' or `lldiv_t' representation
   of the value of NUMER over DENOM. */
extern div_t div (int numer, int denom);
extern ldiv_t ldiv (long int numer, long int denom);
extern lldiv_t lldiv (long long int numer, long long int denom);

/* Convert a string to an integer.  */
extern int atoi (char *c);
/* Convert a string to a long integer.  */
extern long int atol (char *c);
extern long long int atoll (char *__nptr);

/*** unimplemented ***/
#if 0

/* Convert a string to a floating-point number.  */
extern double atof (char *__nptr);
/* Convert a string to a floating-point number.  */
extern double strtod (char *nptr, char **endptr);
/* Likewise for `float' and `long double' sizes of floating-point numbers.  */
extern float strtof (char *nptr, char **endptr);
extern long double strtold (char *nptr, char **endptr);
/* Convert a string to a long integer.  */
extern long int strtol (char *nptr, char **endptr, int base);
/* Convert a string to an unsigned long integer.  */
extern unsigned long int strtoul (char *nptr, char **endptr, int base);
/* Convert a string to a quadword integer.  */
extern long long int strtoll (char *nptr, char **endptr, int base);
/* Convert a string to an unsigned quadword integer.  */
extern unsigned long long int strtoull (char *nptr, char **endptr, int base);

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

/* Return the value of envariable NAME, or NULL if it doesn't exist.  */
extern char *getenv (char *name);
/* Set NAME to VALUE in the environment.
   If REPLACE is nonzero, overwrite an existing value.  */
extern int setenv (char *name, char *value, int replace);

/* Remove the variable NAME from the environment.  */
extern int unsetenv (char *name);

/* Execute the given line as a shell command. */
extern int system (char *command);

/* Return the length of the given multibyte character,
   putting its `wchar_t' representation in *PWC.  */
extern int mbtowc (wchar_t *pwc, char *s, size_t n);
/* Put the multibyte character represented
   by WCHAR in S, returning its length.  */
extern int wctomb (char *s, wchar_t wchar);

/* Convert a multibyte string to a wide char string.  */
extern size_t mbstowcs (wchar_t *pwcs, char *s, size_t n);
/* Convert a wide char string to multibyte string.  */
extern size_t wcstombs (char *s, wchar_t *pwcs, size_t n);

/* Put the 1 minute, 5 minute and 15 minute load averages into the first
   NELEM elements of LOADAVG.  Return the number written (never more than
   three, but may be less than NELEM), or -1 if an error occurred.  */
extern int getloadavg (double loadavg[], int nelem);

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

/* Return the login name of the user.  */
extern char *getlogin (void);

/* Put the name of the current host in no more than LEN bytes of NAME.
   The result is null-terminated if LEN is large enough for the full
   name and the terminator.  */
extern int gethostname (char *__name, size_t __len);

/* Set the name of the current host to NAME, which is LEN bytes long.
   This call is restricted to the super-user.  */
extern int sethostname (const char *__name, size_t __len);

/* Encrypt at most 8 characters from KEY using salt to perturb DES.  */
extern char *crypt (const char *__key, const char *__salt);

/* Encrypt data in BLOCK in place if EDFLAG is zero; otherwise decrypt
   block in place.  */
extern void encrypt (char *__glibc_block, int __edflag);

//uint64_t time();                        // get system time
//void stime(uint64_t utctimestamp);      // set system time

#endif

#endif

#endif /* stdlib.h  */
