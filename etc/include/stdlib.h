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
 * @brief ISO C99 Standard: 7.20 General utilities
 */

#ifndef	_STDLIB_H
#define	_STDLIB_H	1

#include <sys/mman.h>

/* We define these the same for all machines.
   Changes from this to the outside world should be done in `_exit'.  */
#define	EXIT_FAILURE    1   /* Failing exit status.  */
#define	EXIT_SUCCESS    0   /* Successful exit status.  */
/* see also sysexits.h for services */

/* Maximum length of a multibyte character in the current locale.  */
#define	MB_CUR_MAX  4

#ifdef DEBUG
/* debug flags, and user mode kprintf */
#include <sys/debug.h>
extern uint32_t _debug;
extern void dbg_printf(char * fmt, ...);
#endif

extern unsigned char *stdlib_dec(unsigned char *s, uint64_t *v, uint64_t min, uint64_t max);
extern unsigned char *stdlib_hex(unsigned char *s, uint64_t *v, uint64_t min, uint64_t max);

/* Convert a string to an integer.  */
extern int atoi (char *c);
/* Convert a string to a long integer.  */
extern long int atol (char *c);
extern long long int atoll (char *__nptr);

/* memory allocator. Use different macros if you want a different allocator */
#ifndef _BZT_ALLOC
extern void *bzt_alloc(void *arena,size_t a,void *ptr,size_t s,int flag);
extern void bzt_free(void *arena, void *ptr);
extern void bzt_dumpmem(void *arena);
#endif

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

/* Return the `div_t', `ldiv_t' or `lldiv_t' representation
   of the value of NUMER over DENOM. */
extern div_t div (int numer, int denom);
extern ldiv_t ldiv (long int numer, long int denom);
extern lldiv_t lldiv (long long int numer, long long int denom);

/* Do a binary search for KEY in BASE, which consists of NMEMB elements
   of SIZE bytes each, using CMP to perform the comparisons.  */
extern void *bsearch (void *key, void *base, size_t nmemb, size_t size, int (*cmp)(void *, void *));

/* Sort NMEMB elements of BASE, of SIZE bytes each,
   using CMP to perform the comparisons.  */
extern void qsort (void *base, size_t nmemb, size_t size, int (*cmp)(void *, void *));

/* Return the canonical absolute name of file NAME.  If RESOLVED is
   null, the result is malloc'd; otherwise, if the canonical name is
   PATH_MAX chars or more, returns null with `errno' set to
   ENAMETOOLONG; if the name fits in fewer than PATH_MAX chars,
   returns the name in RESOLVED.  */
extern char *realpath (char *name, char *resolved);

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

/* Return the value of envariable NAME, or NULL if it doesn't exist.  */
extern char *getenv (char *name);
/* Set NAME to VALUE in the environment.
   If REPLACE is nonzero, overwrite an existing value.  */
extern int setenv (char *name, char *value, int replace);

/* Remove the variable NAME from the environment.  */
extern int unsetenv (char *name);

/* Execute the given line as a shell command. */
extern int system (char *command);

/* Return a malloc'd string containing the canonical absolute name of the
   existing named file.  */
extern char *canonicalize_file_name (char *name);

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

#endif

#endif /* stdlib.h  */
