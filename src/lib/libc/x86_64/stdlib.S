/*
 * lib/libc/x86_64/stdlib.S
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
 * @brief Low level library implementation to interact with the kernel, see stdlib.h
 */

#define _AS 1
#include <limits.h>
#include <syscall.h>
#include <errno.h>
#include "../../../core/x86_64/tcb.h"

.type exit, STT_FUNC

.global mq_call
.global mq_send
.global mq_recv
.global mq_ismsg
.global p2pcpy
.global lockacquire
.global lockrelease
.global dl
.global yield
.global setirq
.global mmap
.global munmap
.global p2pcpy
.global sleep
.global usleep
.global meminfo
.global getentropy
.global abort
.global exit
.global srand
.global rand
.global mblen
.global div
.global ldiv
.global lldiv
/* don't use directly, use mq_dispatch() instead */
.global mq_dispatchcall
#if DEBUG
.global dbg_printf
#endif

.section .text
/*****************************************************************************
 *                             OS/Z system calls                             *
 *****************************************************************************/
 
/**
 * non-blocking, send message
 *
 * void mq_send(task,event,arg0,arg1,arg2,arg3,arg4,arg5)
 */
mq_send:
mymq_send:
    movq    %rdi, %rax
    shlq    $16, %rax
    movw    %si, %ax
    movq    %rdx, %rdi
    movq    %rcx, %rsi
    movq    %r8, %rdx
    movq    %r9, %rbx
    movq    8(%rsp), %r8
    movq    16(%rsp), %r9
    movq    24(%rsp), %r10
    /* this will call msg_sends() in core */
    syscall
    movq    tcb_serial, %rax
    call    myseterrno
    ret

/**
 * blocking, send a request and receive result
 *
 * msg_t *mq_call(task,event,arg0,arg1,arg2,arg3,arg4,arg5)
 */
mq_call:
mymq_call:
    /* send request */
1:  call    mymq_send
    cmpb    $EAGAIN, %bl
    jne     1f
    call    myyield
    jmp     1b
1:  /* receive result */
    orb     %bl, %bl
    jnz     1f
    call    mymq_recv
1:  ret

/**
 * blocking if there's no message waiting
 *
 * msg_t *mq_recv()
 */
mq_recv:
mymq_recv:
    /* block until we receive something */
    xorq    %rax, %rax
    movw    $SYS_recv, %ax
    syscall
    /* errno from recv call itself */
    orb     %bl, %bl
    jnz     1f
    /* nack message sends errno in argument */
    cmpw    $SYS_nack, %di
    jne     1f
2:  movb    8(%rax), %bl
1:  call    myseterrno
    ret

/**
 * returns message serial or 0 if no message waiting
 *
 * uint64_t mq_ismsg()
 */
mq_ismsg:
    # non-blocking is there a message?
    movq    MQ_ADDRESS, %rax
    cmpq    MQ_ADDRESS+8, %rax
    je      1f
    movq    56(%rax), %rax
    orq     %rax, %rax
    jnz     1f
    incb    %al
1:  ret

/*****************************************************************************
 *                      OS/Z standard library functions                      *
 *****************************************************************************/

#if DEBUG
dbg_printf:
    xorq    %rax, %rax
    movq    %rcx, %rbx
    syscall
    ret
#endif

/* Don't use directly */
mq_dispatchcall:
    /* rax=*symtable[MSG_FUNC(msg->evt)](msg->arg0, msg->arg1, msg->arg2, msg->arg3, msg->arg4, msg->arg5) */
    movq    8(%rsp), %rax
    call    *%rax
    ret

/* Don't use directly. in: rbx: errno */
myseterrno:
    push    %rax
    push    %rdx
    push    %rdi
    movq    %rbx, %rdi
    call    seterr@PLT
    pop     %rdi
    pop     %rdx
    pop     %rax
    ret

/**
 * void p2pcpy (pid_t dst, void* dest, void *src, size_t len);
 */
p2pcpy:
    xorq    %rax, %rax
    movb    $SYS_p2pcpy, %al
    movq    %rcx, %rbx
    syscall
    call    myseterrno
    ret

/**
 * Set a bit. Return if it was zero, yield otherwise.
 *
 * void lockacquire(int bit, uint64_t *mem)
 */
lockacquire:
1:  lock
    btsq    %rdi, (%rsi)
    jnc     1f
    call    myyield
    jmp     1b
1:  ret

/**
 * Clear a bit
 *
 * void lockrelease(int bit, uint64_t *mem)
 */
lockrelease:
    lock
    btrq    %rdi, (%rsi)
    ret

/**
 * Dynamically link a shared object
 *
 * void *dl(so, sym)
 */
dl:
    # rdi: pointer to .so file path
    # rsi: pointer to symbol name
    xorq    %rax, %rax
    movb    $SYS_dl, %al
    syscall
    call    mymq_recv
    mov     8(%rax), %rax
    ret

/**
 * Give up remaining CPU time in this quantum
 *
 * void yield()
 */
yield:
myyield:
    xorq    %rax, %rax
    movb    $SYS_sched_yield, %al
    syscall
    ret

/**
 * void *mmap (void *addr, size_t len, int prot, int flags, int fd, off_t offset);
 */
mmap:
    movq    %rcx, %rbx
    xorq    %rax, %rax
    movb    $SYS_mmap, %al
    syscall
    call    myseterrno
    ret

/**
 * int munmap (void *addr, size_t len);
 */
munmap:
    xorq    %rax, %rax
    movb    $SYS_munmap, %al
    syscall
    call    myseterrno
    ret

msync:
    xorq    %rax, %rax
    movb    $SYS_msync, %al
    syscall
    call    myseterrno
    ret

mlock:
    xorq    %rax, %rax
    movb    $SYS_mlock, %al
    syscall
    call    myseterrno
    ret

munlock:
    xorq    %rax, %rax
    movb    $SYS_munlock, %al
    syscall
    call    myseterrno
    ret

/**
 * void setirq(uint8_t irq) - ONLY for drivers
 */
setirq:
    andq    $0xFF, %rdi
    xorq    %rax, %rax
    movb    $SYS_setirq, %al
    syscall
    call    myseterrno
    ret

/**
 * void sleep(uint64_t sec)
 */
sleep:
    xorq    %rax, %rax
    movb    $SYS_alarm, %al
    syscall
    call    myseterrno
    ret

/**
 * void usleep(uint64_t usec)
 */
usleep:
    xorq    %rdx, %rdx
    movq    %rdi, %rax
    movq    $1000000,%rcx
    divq    %rcx
    /* sec != 0? */
    orq     %rdx, %rdx
    jnz     1f
    /* usec < alarmstep? */
    cmpq    _alarmstep@GOTPCREL(%rip), %rsi
    ja      1f
    /* loops = usec*bogomips/1000000 */
    movq    _bogomips@GOTPCREL(%rip), %rbx
    mulq    %rbx
    divq    %rcx
2:  pause
    decq    %rax
    orq     %rax, %rax
    jnz     2b
    ret
1:  movq    %rax, %rdi
    movq    %rdx, %rsi
    xorq    %rax, %rax
    movb    $SYS_alarm, %al
    syscall
    call    myseterrno
    ret

/**
 * msg_t *meminfo()
 */
meminfo:
    xorq    %rax, %rax
    movb    $SYS_meminfo, %al
    syscall
    call    mymq_recv
    movq    16(%rax), %rdx
    movq    8(%rax), %rax
    call    myseterrno
    ret

/**
 * int getentropy (void *__buffer, size_t __length);
 */
getentropy:
    /* TODO: getentropy */
    ret

/**
 * Exit process.
 *
 * void exit(status)
 */
exit:
    pushq   %rdi
    callq   _atexit@PLT
    popq    %rdi
    xorq    %rax, %rax
    xorq    %rsi, %rsi
    movb    $SYS_exit, %al
    syscall
    /* fall into abort code. If we return something really bad happened */
/**
 * Abort execution and generate a core-dump.
 *
 * void abort() 
 */
abort:
    xorq    %rax, %rax
    xorq    %rdi, %rdi
    movb    $EFAULT, %dil
    xorq    %rsi, %rsi
    incb    %sil
    movb    $SYS_exit, %al
    syscall
    ret

/**
 * initialize random with seed
 *
 * void srand(uint64_t seed)
 */
srand:
    xorq    %rax, %rax
    movb    $SYS_srand, %al
    syscall
    ret

/**
 * Fills up almost every register with random
 *
 * uint64_t rand()
 */
rand:
    xorq    %rax, %rax
    movb    $SYS_rand, %al
    syscall
    ret

/**
 * int mblen (char *s, size_t n);
 */
mblen:
    cld
    movq    %rsi, %rdx
    movq    %rdi, %rsi
    xor     %rcx, %rcx
    orq     %rsi, %rsi
    jz      2f
    decq    %rcx
1:  lodsb
    incq    %rcx
    btw     $7, %ax
    jnc     3f
    incq    %rsi
    movb    %al, %ah
    shrb    $5, %al
    cmpb    $0b110, %al
    je      3f
    incq    %rsi
    movb    %ah, %al
    shrb    $4, %al
    cmpb    $0b1110, %al
    je      3f
    incq    %rsi
3:  decq    %rdx
    jz      2f
    or      %al, %al
    jnz     1b
2:  movq    %rcx, %rax
    ret

/**
 * div_t div (int numer, int denom);
 */
div:
    xorq    %rax, %rax
    movl    %edi, %eax
    xorl    %edx, %edx
    divl    %esi
    shlq    $32, %rdx
    addq    %rdx, %rax
    ret

/**
 * ldiv_t ldiv (long int numer, long int denom);
 * lldiv_t lldiv (long long int numer, long long int denom);
 */
ldiv:
lldiv:
    movq    %rdi, %rax
    xorq    %rdx, %rdx
    divq    %rsi
    ret
