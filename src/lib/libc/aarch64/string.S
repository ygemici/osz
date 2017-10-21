/*
 * lib/libc/aarch64/string.S
 * 
 * Copyright 2016 CC-by-nc-sa-4.0 bztsrc@github
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
 * @brief Low level string operations.
 */

/* All functions must be direction flag independent, and must behave well on NULL input.
 * What's more str* functions must be UTF-8 aware. */

.global memzero
.global memcpy
.global memmove
.global memset
.global memcmp
.global memchr
.global ffs
.global strcpy
.global strncpy
.global strcat
.global strncat
.global strcmp
.global strncmp
.global strchr
.global strrchr
.global strlen
.global strnlen
.global mbstrlen
.global strcasecmp
.global strncasecmp

.section .text

/**
 * Set N bytes of S to zero.
 *
 * void *memzero (void *s, size_t n);
 */
memzero:
2:  ret

/**
 * Copy N bytes of SRC to DEST, guaranteeing
 * correct behavior for overlapping strings. Shares code with memcpy()
 *
 * void *memmove (void *dest, void *src, size_t n);
 */
memmove:
/**
 * Copy N bytes of SRC to DEST.
 *
 * void *memcpy (void *dest, void *src, size_t n);
 */
memcpy:
2:  ret

/**
 * Set N bytes of S to C.
 *
 * void *memset (void *s, int c, size_t n);
 */
memset:
2:  ret

/**
 * Compare N bytes of S1 and S2.
 *
 * int memcmp (void *s1, void *s2, size_t n);
 */
memcmp:
1:  ret

/**
 * These are not UTF-8 safe
 *
 * void *memchr (void *s, int c, size_t n);
 */
memchr:
2:  ret

/**
 * void *memrchr (void *s, int c, size_t n);
 */
memrchr:
1:  ret

/**
 * Return the position of the first bit set in I, or 0 if none are set.
 * The least-significant bit is position 1, the most-significant 64.
 *
 * uint64_t ffs (uint64_t i);
 */
ffs:
1:  ret

/**
 * Append SRC onto DEST. Shares code with strcpy()
 *
 * char *strcat (char *dest, char *src);
 */
strcat:
/**
 * Copy SRC to DEST.
 *
 * char *strcpy (char *dest, char *src);
 */
strcpy:
    ret

/**
 * Append no more than N characters from SRC onto DEST. Shares code with strncpy()
 *
 * char *strncat (char *dest, char *src, size_t n);
 */
strncat:
/**
 * Copy no more than N characters of SRC to DEST.
 *
 * char *strncpy (char *dest, char *src, size_t n);
 */
strncpy:
    ret

/**
 * Compare S1 and S2.
 *
 * int strcmp (char *s1, char *s2);
 */
strcmp:
    ret
    
/**
 * Compare N characters of S1 and S2.
 *
 * int strncmp (char *s1, char *s2, size_t n);
 */
strncmp:
    ret

/**
 * size_t strlen (char *s);
 */
strlen:
    ret

/**
 * size_t strnlen (char *s, size_t maxlen);
 */
strnlen:
    ret

/**
 * Return the number of multibytes (UTF-8 sequences) in S.
 * For mbstrnlen(), see mblen() in stdlib.h
 *
 * size_t mbstrlen (char *s);
 */
mbstrlen:
    ret

/**
 * NOTE: utf-8 multibyte safe
 *
 * char *strchr (char *s, int c);
 */
strchr:
3:  ret

/**
 * NOTE: utf-8 multibyte safe
 *
 * char *strrchr (char *s, int c);
 */
strrchr:
3:  ret

/**
 * Compare S1 and S2, ignoring case.
 *
 * int strcasecmp (char *s1, char *s2);
 */
strcasecmp:
/**
 * Compare no more than N chars of S1 and S2, ignoring case.
 *
 * int strncasecmp (char *s1, char *s2, size_t n);
 */
strncasecmp:
    ret

/* IN: ebx: char, OUT: eax: lower char, must save rcx, rdx */
tolower:
1:  ret
    
