/*
 * core/aarch64/libk.S
 *
 * Copyright 2017 CC-by-nc-sa bztsrc@github
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
#include "arch.h"

.section .text
.global idle
.type idle, STT_FUNC
.global kmap
.type kmap, STT_FUNC
.global kmap_init
.type kmap_init, STT_FUNC
.global kmap_mq
.type kmap_mq, STT_FUNC
.global kmap_buf
.type kmap_buf, STT_FUNC
.global kmap_getpte
.type kmap_getpte, STT_FUNC
.global kstrcmp
.type kstrcmp, STT_FUNC
.global ksend
.type ksend, STT_FUNC
.global kwaitkey
.type kwaitkey, STT_FUNC
.global klockacquire
.type klockacquire, STT_FUNC
.global klockrelease
.type klockrelease, STT_FUNC

/* idle task's code */
idle:
    wfi
    b      idle

/* acquire a lock, for multi core system */
klockacquire:
1:  ret

/* release a lock */
klockrelease:
    ret

kstrcmp:
2:  ret

/* rdi: virtual address
   rsi: physical address
   rdx: access rights
*/
kmap:
    ret

kmap_init:
    ret

/* map a message queue temporarily
   IN rdi: tcb->memroot */
kmap_mq:
    ret

/* map a buffer temporarily (up to 2M)
   IN: rdi: tcb->memroot, rsi: pointer, rdx: size */
kmap_buf:
    ret

/* IN rdi: virtual address
 * OUT rax: address of page entry in PT */
kmap_getpte:
//    movq    %cr3, %rdx
kmap_getpte2:
    ret

/* send a message to a message queue. This has to be effective
   and I didn't liked the code gcc generated, that's why it's
   asm, otherwise this routine is not platform specific.
    IN: rdi: mq, rsi: event, rdx: arg0, rcx: arg1, r8: arg2
    OUT: true on success
*/
ksend:
    ret

/* an early implementation, called by kprintf */
kwaitkey:
    ret
