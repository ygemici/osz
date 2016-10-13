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
 * @brief Low base library to interact with the kernel at object level
 */
.include "sys/core.h"

.extern main
.global _start
.global exit
.global clcall
.global clsend
.global clrecv
.global clismg
.intel_syntax noprefix

.section .text
_start:
	call    main
	mov     rcx, rax
	jmp     1f
exit:
	mov     rcx, qword [rsp+8]
1:	mov     ebx, 60         # SYS_exit
	mov     eax, 0x6C6C6163 # 'call'
	syscall
	ret

clcall:
	# blocking
	mov     ebx, dword [rsp+8]
	mov     eax, 0x6C6C6163 # 'call'
	syscall
	ret

clsend:
	# non-blocking
	mov     eax, 0x646E6573 # 'send'
	syscall
	ret

clrecv:
	# blocking
	mov     eax, 0x76636572 # 'recv'
	syscall
	ret

clismg:
	# non-blocking
	mov     eax, 0x676D7369 # 'ismg'
	syscall
	ret

cldl:
	#dynamically link a class
	mov     ecx, dword [rsp+16] # handler
	mov     ebx, dword [rsp+8] # elf binary with .cl section
	mov     eax, 0x6B6E696C # 'link'
	syscall
	ret
