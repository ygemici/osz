/*
 * fs/cache.h
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
 * @brief Cache definitions
 */
#include <osZ.h>

#define CACHEDIRNAME 56
typedef struct {
    ino_t inode;
    char name[CACHEDIRNAME];
} cachedir_t;

extern uint64_t ncachedir;
extern cachedir_t *cachedir;

typedef struct {
} cache_t;

extern uint64_t ncache;
extern cachedir_t *cache;

extern void cache_dir(char*name,ino_t inode);
extern void cache_init();
#if DEBUG
extern void cache_dump();
#endif
