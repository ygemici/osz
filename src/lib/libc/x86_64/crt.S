/*
 * libc/crt.S
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
 * @brief Zero level C runtime (crt0)
 *
 * Calling convention: SystemV AMD64 ABI
 *  integer or pointer RDI, RSI, RDX, RCX, R8, R9
 *  floating point XMM0-XMM7
 *  preserve RBP, RBX
 */
.include "sys/core.h"

.extern main
.global _start
.global exit
.global atexit
.intel_syntax noprefix

.section .text
_start:
	mov		rax, 6f
	mov		qword [5f], rax
	xor		rax, rax
	call    main
	mov     rcx, rax
	jmp     1f
exit:
	mov     rcx, qword [rsp+8]
1:	push	rcx
	mov		rsi, 6f
2:	cmp		rsi, 7f
	je		3f
	lodsq
	or		rax, rax
	jz		3f
	push	rsi
	call	rax
	pop		rsi
	jmp		2b
3:	pop		rcx
	mov     ebx, 60			# SYS_exit
	mov     eax, 0x6C6C6163 # 'call'
	syscall
	ret
atexit:
	mov     rax, qword [rsp+8]
	or		rax, rax
	jz		4f
	mov		rbx, qword [5f]
	cmp		rbx, 7f
	je		4f
	cmp		rbx, qword [5f]
	je		4f
	mov		qword [rbx], rax
	add		rbx, 8
	mov		qword [5f], rbx
4:	ret

.section .data
5:
.quad 6f
6:
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
.quad 0
7:

