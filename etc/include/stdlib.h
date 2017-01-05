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
 * @brief ISO C99 Standard: 7.20 General utilities
 */

#ifndef	_STDLIB_H
#define	_STDLIB_H	1

/* We define these the same for all machines.
   Changes from this to the outside world should be done in `_exit'.  */
#define	EXIT_FAILURE	1	/* Failing exit status.  */
#define	EXIT_SUCCESS	0	/* Successful exit status.  */

#if 0

/* Maximum length of a multibyte character in the current locale.  */
#define	MB_CUR_MAX	(__ctype_get_mb_cur_max ())
extern size_t __ctype_get_mb_cur_max (void);

/* Convert a string to a floating-point number.  */
extern double atof (char *__nptr);
/* Convert a string to an integer.  */
extern int atoi (char *__nptr);
/* Convert a string to a long integer.  */
extern long int atol (char *__nptr);
extern long long int atoll (char *__nptr);
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

/* Return a random integer between 0 and RAND_MAX inclusive.  */
extern int rand (void);
/* Seed the random number generator with the given number.  */
extern void srand (unsigned int seed);

/* Allocate SIZE bytes of memory.  */
extern void *malloc (size_t size);
/* Allocate NMEMB elements of SIZE bytes each, all initialized to 0.  */
extern void *calloc (size_t nmemb, size_t size);

/* Re-allocate the previously allocated block
   in PTR, making the new block SIZE bytes long.  */
extern void *realloc (void *__ptr, size_t __size);
/* Free a block allocated by `malloc', `realloc' or `calloc'.  */
extern void free (void *ptr);
/* ISO C variant of aligned allocation.  */
extern void *aligned_alloc (size_t alignment, size_t size);

/* Abort execution and generate a core-dump.  */
extern void abort (void)  __attribute__ ((__noreturn__));

/* Register a function to be called when `exit' is called.  */
extern int atexit (void (*func) (void));

/* Call all functions registered with `atexit' and `on_exit',
   in the reverse of the order in which they were registered,
   perform stdio cleanup, and terminate program execution with STATUS.  */
extern void exit (int __status) __attribute__ ((__noreturn__));

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

/* Return the canonical absolute name of file NAME.  If RESOLVED is
   null, the result is malloc'd; otherwise, if the canonical name is
   PATH_MAX chars or more, returns null with `errno' set to
   ENAMETOOLONG; if the name fits in fewer than PATH_MAX chars,
   returns the name in RESOLVED.  */
extern char *realpath (char *name, char *resolved);

typedef int (*__compar_fn_t) (const void *, const void *);

/* Do a binary search for KEY in BASE, which consists of NMEMB elements
   of SIZE bytes each, using COMPAR to perform the comparisons.  */
extern void *bsearch (void *key, void *base, size_t nmemb, size_t size, __compar_fn_t compar);

/* Sort NMEMB elements of BASE, of SIZE bytes each,
   using COMPAR to perform the comparisons.  */
extern void qsort (void *base, size_t nmemb, size_t size, __compar_fn_t compar);

/* Return the absolute value of X.  */
extern int abs (int x);
extern long int labs (long int x);
extern long long int llabs (long long int x);
/* Return the `div_t', `ldiv_t' or `lldiv_t' representation
   of the value of NUMER over DENOM. */
extern div_t div (int numer, int denom);
extern ldiv_t ldiv (long int numer, long int denom);
extern lldiv_t lldiv (long long int numer, long long int denom);

/* Return the length of the multibyte character
   in S, which is no longer than N.  */
extern int mblen (char *s, size_t n);
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
