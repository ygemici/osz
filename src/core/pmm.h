/*
 * core/pmm.h
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
 * @brief Physical memory manager
 */

#define OSZ_PMM_MAGIC "FMEM"
#define OSZ_PMM_MAGICH 0x4d454d46

typedef struct {
    uint64_t base;
    uint64_t size;
} pmm_entry_t;

typedef struct {
    uint32_t magic;
    uint32_t size;
    uint64_t totalpages;
    uint64_t freepages;
    pmm_entry_t *entries;
    void *bss;
    void *bss_end;
} __attribute__((packed)) pmm_t;
