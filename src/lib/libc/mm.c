/*
 * libc/mm.c
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
 * @brief Memory management
 */

#include <osZ.h>

/* Allocate SIZE bytes of memory.  */
void *malloc (size_t __size){ return NULL; }
/* Allocate NMEMB elements of SIZE bytes each, all initialized to 0.  */
void *calloc (size_t __nmemb, size_t __size){ return NULL; }
/* Re-allocate the previously allocated block
   in PTR, making the new block SIZE bytes long.  */
void *realloc (void *__ptr, size_t __size){ return NULL; }
/* Free a block allocated by `malloc', `realloc' or `calloc'.  */
void free (void *__ptr){}
/* ISO C variant of aligned allocation.  */
void *aligned_alloc (size_t __alignment, size_t __size){ return NULL; }


