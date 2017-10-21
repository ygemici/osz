/*
 * core/data.c
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
 * @brief Common core data
 */

#include "arch.h"

/* number of cores */
uint32_t numcores = 1;
/* current fps counter */
uint64_t isr_currfps;
uint64_t isr_lastfps;
/* locks, timer ticks, random seed, system tables */
uint64_t locks, ticks[4], srand[4], systables[8];
/* system fault code */
uint8_t sys_fault;
/* memory mappings */
phy_t idle_mapping, core_mapping, identity_mapping;
/* pointer to tmpmap in PT */
uint64_t *kmap_tmp;
/* memory allocated for relocation addresses */
rela_t *relas;
