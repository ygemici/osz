/*
 * libc/x86_64/syscall.S
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
 * @brief Low base library to interact with the kernel at object level
 */

#define _AS 1
#include <tcb.h>

.global clcall
.global clsend
.global clrecv
.global clismg
.global cldl

/* dest: pid_t<<12 + msg type */
.section .text
/* uint64_t clcall(dest,arg0,arg1,arg2) */
clcall:
    # blocking, send a request and receive result
    # rdi: destination pid + message type
    movl     $0x6C6C6163, %eax # 'call'
    syscall
    ret

/* void clsend(dest,arg0,arg1,arg2) */
clsend:
    # non-blocking send message
    movl     $0x646E6573, %eax # 'send'
    syscall
    ret

/* uint64_t clrecv(from) */
clrecv:
    # blocking receive message
    movl     $0x76636572, %eax # 'recv'
    syscall
    ret

/* uint64_t clismg() */
clismg:
    # non-blocking is there a message?
    movq     OSZ_tcb_evtq_ptr, %rax
    subq     OSZ_tcb_evtq_endptr, %rax
    ret

/* void *cldl(&symtable,objectfile) */
cldl:
    # dynamically link a shared object
    # rdi: pointer to symtable to fill
    # rsi: pointer to .so file
    movl     $0x6B6E696C, %eax # 'link'
    syscall
    ret
