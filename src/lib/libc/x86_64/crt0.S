/*
 * libc/x86_64/crt0.S
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
 * @brief Zero level C runtime (crt0-osZ-x86_64)
 *
 * Calling convention: SystemV AMD64 ABI
 *  integer or pointer RDI, RSI, RDX, RCX, R8, R9
 *  floating point XMM0-XMM7
 *  preserve RBP, RBX
 */

.extern main
.extern exit
.global _start

.section .crt0

    /* initial RIP on thread execution, first byte of text segment */
    retq

.section .text

_start:
    /* restore argc, argv as they may be ruined by shlib's _init */
    movq    (%rsp), %rdi
    movq    8(%rsp), %rsi
    /* call main(argc,argv) function */
    call    main
    /* exit thread */
    movq    %rax, %rdi
    jmp    exit
