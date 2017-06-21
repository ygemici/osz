/*
 * string.h
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
 * @brief ISO C99 Standard: 7.21 String handling
 */

/*
 *	ISO C99 Standard: 7.21 String handling	<string.h>
 */

#ifndef	_STRING_H
#define	_STRING_H	1

#include <osZ.h>

/* Set N bytes of S to zero.  */
extern void *memzero (void *s, size_t n);
/* Copy N bytes of SRC to DEST.  */
extern void *memcpy (void *dest, void *src, size_t n);
/* Copy N bytes of SRC to DEST, guaranteeing
   correct behavior for overlapping strings.  */
extern void *memmove (void *dest, void *src, size_t n);
/* Set N bytes of S to C.  */
extern void *memset (void *s, int c, size_t n);

/* Compare N bytes of S1 and S2.  */
extern int memcmp (void *s1, void *s2, size_t n);
/* These are not UTF-8 safe */
extern void *memchr (void *s, int c, size_t n);
extern void *memrchr (void *s, int c, size_t n);
/* Find the first occurrence of NEEDLE in HAYSTACK.
   NEEDLE is NEEDLELEN bytes long;
   HAYSTACK is HAYSTACKLEN bytes long.  */
extern void *memmem (void *haystack, size_t haystacklen, void *needle, size_t needlelen);

/* Copy SRC to DEST.  */
extern char *strcpy (char *dest, char *src);
/* Copy no more than N characters of SRC to DEST.  */
extern char *strncpy (char *dest, char *src, size_t n);

/* Append SRC onto DEST.  */
extern char *strcat (char *dest, char *src);
/* Append no more than N characters from SRC onto DEST.  */
extern char *strncat (char *dest, char *src, size_t n);

/* Compare S1 and S2.  */
extern int strcmp (char *s1, char *s2);
/* Compare N characters of S1 and S2.  */
extern int strncmp (char *s1, char *s2, size_t n);

/* Duplicate S, returning an identical malloc'd string.  */
extern char *strdup (char *s);
extern char *strndup (char *s, size_t n);

/* In OS/Z, these are UTF-8 safe */
extern char *strchr (char *s, int c);
extern char *strrchr (char *s, int c);
/* Find the first occurrence of NEEDLE in HAYSTACK.  */
extern char *strstr (char *haystack, char *needle);

/* Divide S into tokens separated by characters in DELIM.  */
extern char *strtok (char *s, char *delim);
extern char *strtok_r (char *s, char *delim, char **ptr);
/* Similar to `strstr' but this function ignores the case of both strings.  */
extern char *strcasestr (char *haystack, char *needle);
extern size_t strlen (char *s);
extern size_t strnlen (char *s, size_t maxlen);
/* Return the number of multibytes (UTF-8 sequences) in S. */
extern size_t mbstrlen (char *s);
/* see stdlib's mblen()
extern size_t mbstrnlen (char *s, size_t maxlen); */
/* Return a string describing the meaning of the `errno' code in ERRNUM.  */
extern char *strerror (int errnum);
/* Return the position of the first bit set in I, or 0 if none are set.
   The least-significant bit is position 1, the most-significant 64.  */
extern uint64_t ffs (uint64_t i);
/* Compare S1 and S2, ignoring case.  */
extern int strcasecmp (char *s1, char *s2);
/* Compare no more than N chars of S1 and S2, ignoring case.  */
extern int strncasecmp (char *s1, char *s2, size_t n);
/* Return the next DELIM-delimited token from *STRINGP,
   terminating it with a '\0', and update *STRINGP to point past it.  */
extern char *strsep (char **stringp, char *delim);
/* Return a string describing the meaning of the signal number in SIG.  */
extern char *strsignal (int sig);
/* Return filename part of path */
extern char *basename (char *s);
/* Return directory part of path */
extern char *dirname (char *s);

#endif /* string.h  */
