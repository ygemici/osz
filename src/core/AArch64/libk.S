/*
 * core/AArch64/libk.S
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
 * @brief low level kernel routines for the core
 */
#define _AS 1

.section .text
.global kpanic
.global kmemcpy
.global kmemset
.global kmemcmp
.global kmap
.global kmap_init
.global kstrcpy
.global kstrlen
.global kstrcmp
.global ksend

/* kernel panic */
kpanic:
    ret

kpanicprefix:

/* misc memory functions */
kmemcpy:
    ret

kmemset:
    ret

kmemcmp:
    ret

kstrcpy:
    ret

kstrlen:
    ret

kstrcmp:
    ret

kmap:
    ret

kmap_init:
    ret

/* send a message to a message queue. This has to be effective
   and I didn't like the code gcc generated, that's why it's
   asm, otherwise this routine is not platform specific. */
ksend:
    ret
