/* Copyright (C) 1991-2016 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

/*
 *	ISO C99 Standard: 7.21 String handling	<string.h>
 */

#ifndef	_STRING_H
#define	_STRING_H	1

#include <osZ.h>

/* Copy N bytes of SRC to DEST.  */
extern void *memcpy (void *__restrict __dest, const void *__restrict __src,
		     size_t __n);
/* Copy N bytes of SRC to DEST, guaranteeing
   correct behavior for overlapping strings.  */
extern void *memmove (void *__dest, const void *__src, size_t __n);
/* Set N bytes of S to C.  */
extern void *memset (void *__s, int __c, size_t __n);

/* Compare N bytes of S1 and S2.  */
extern int memcmp (const void *__s1, const void *__s2, size_t __n);

extern void *memchr (void *__s, int __c, size_t __n);

/* Copy SRC to DEST.  */
extern char *strcpy (char *__restrict __dest, const char *__restrict __src);
/* Copy no more than N characters of SRC to DEST.  */
extern char *strncpy (char *__restrict __dest,
		      const char *__restrict __src, size_t __n);

/* Append SRC onto DEST.  */
extern char *strcat (char *__restrict __dest, const char *__restrict __src);
/* Append no more than N characters from SRC onto DEST.  */
extern char *strncat (char *__restrict __dest, const char *__restrict __src,
		      size_t __n);

/* Compare S1 and S2.  */
extern int strcmp (const char *__s1, const char *__s2);
/* Compare N characters of S1 and S2.  */
extern int strncmp (const char *__s1, const char *__s2, size_t __n);

/* Duplicate S, returning an identical malloc'd string.  */
extern char *strdup (const char *__s);
extern char *strndup (const char *__string, size_t __n);
extern char *strchr (char *__s, int __c);
extern const char *strchr (const char *__s, int __c);
extern char *strrchr (char *__s, int __c);
extern const char *strrchr (const char *__s, int __c);
extern char *strpbrk (char *__s, const char *__accept);
extern const char *strpbrk (const char *__s, const char *__accept);
extern char *strstr (char *__haystack, const char *__needle);
extern const char *strstr (const char *__haystack, const char *__needle);
/* Divide S into tokens separated by characters in DELIM.  */
extern char *strtok (char *__restrict __s, const char *__restrict __delim);
extern char *strtok_r (char *__restrict __s, const char *__restrict __delim,
		       char **__restrict __save_ptr);
/* Similar to `strstr' but this function ignores the case of both strings.  */
extern char *strcasestr (char *__haystack, const char *__needle);
extern const char *strcasestr (const char *__haystack,
				     const char *__needle);
/* Find the first occurrence of NEEDLE in HAYSTACK.
   NEEDLE is NEEDLELEN bytes long;
   HAYSTACK is HAYSTACKLEN bytes long.  */
extern void *memmem (const void *__haystack, size_t __haystacklen,
		     const void *__needle, size_t __needlelen);

extern size_t strlen (const char *__s);
extern size_t strnlen (const char *__string, size_t __maxlen);
/* Return a string describing the meaning of the `errno' code in ERRNUM.  */
extern char *strerror (int __errnum);
/* Return the position of the first bit set in I, or 0 if none are set.
   The least-significant bit is position 1, the most-significant 64.  */
extern uint64_t ffs (uint64_t __i);
/* Compare S1 and S2, ignoring case.  */
extern int strcasecmp (const char *__s1, const char *__s2);
/* Compare no more than N chars of S1 and S2, ignoring case.  */
extern int strncasecmp (const char *__s1, const char *__s2, size_t __n);
/* Return the next DELIM-delimited token from *STRINGP,
   terminating it with a '\0', and update *STRINGP to point past it.  */
extern char *strsep (char **__restrict __stringp,
		     const char *__restrict __delim);
/* Return a string describing the meaning of the signal number in SIG.  */
extern char *strsignal (int __sig);
extern char *basename (char *__filename);
extern char *dirname (char *__filename);

#endif /* string.h  */
