OS/Z Message Queues
===================

Preface
-------

The main inter process communication form of OS/Z threads are message queues.
It's a so essential type that it's defined in [etc/include/sys/types.h](https://github.com/bztsrc/osz/blob/master/etc/include/sys/types.h).

```c
msg_t *mq;
```

The first item in the array is the header

```c
typedef struct {
    uint64_t mq_start;
    uint64_t mq_end;
    uint64_t mq_size;
    pid_t mq_recvfrom;
    uint64_t mq_buffstart;
    uint64_t mq_buffend;
    uint64_t mq_buffsize;
    uint64_t reserved;
} msghdr_t;

msghdr = (msghdr_t *)mq[0];
```

The other items form a circular buffer for a FIFO queue. The size of that buffer is msghdr_t.mq_size - 1.
If msghdr_t.mq_recvfrom is set, then the queue is exclusively allocated to that thread: other
threads sending to the queue will receive errno EAGAIN. When it's zero, all threads can send messages into the queue.

Because the exact implementation of msghdr_t is irrelevant to the queue's user, it scope is core only and defined
in [src/core/msg.h](https://github.com/bztsrc/osz/blob/master/src/core/msg.h).

User level library
------------------

Normally you won't see a thing about message queues. The libc library hides all the details from you, so you just
use printf() and fopen() etc. as usual.

### Low level user Library

Only five functions, variations on synchronisation. They are provided by `libc`, and defined in [etc/include/syscall.h](https://github.com/bztsrc/osz/blob/master/etc/include/syscall.h). Wrappers around user mode syscalls.

```c
/* async, send a message (non-blocking, except dest queue is full) */
bool_t mq_send(pid_t dst, uint64_t func, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5);
/* sync, send a message and receive result (blocking) */
msg_t *mq_call(pid_t dst, uint64_t func, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5);
/* async, is there a message? (non-blocking) */
bool_t mq_ismsg();
/* sync, wait until there's a message (blocking) */
msg_t *mq_recv();
/* sync, dispatch events (blocking, noreturn) */
void mq_dispatch();
```
While mq_call() is a sequence of mq_send() and my_recv() calls, mq_dispatch() is quite the opposite: it first
receives a message with mq_recv(), calls the handler for it, and then uses mq_send() to send the result back.

Supervisor Mode (Ring 0)
------------------------

Core can't use libc, it has it's own message queue implementation. Two functions in [src/core/msg.c](https://github.com/bztsrc/osz/blob/master/src/core/msg.c):

```c
bool_t msg_send(pid_t thread, uint64_t event, void *ptr, size_t size, uint64_t magic);
bool_t msg_sends(pid_t thread, uint64_t event, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5);
```

Those are variations on scalar and reference arguments. You can also
pass a system service number as pid, they can be found in [etc/include/syscall.h](https://github.com/bztsrc/osz/blob/master/etc/include/syscall.h).

If magic is given it can be a type hint on what's ptr is pointing to. It's optional as most events accept
only one kind of reference.

### Low Level

The lowest level of message sending can be found in [src/core/(platform)/libk.S](https://github.com/bztsrc/osz/blob/master/src/core/x86_64/libk.S) and it's a non-blocking call.

```c
void ksend(msghdr_t *mqhdr, uint64_t event, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5);
```

It's an effective assembly implementation of copying msg_t into the queue and handle start / end indeces.

User Mode (Ring 3)
------------------

From userspace the queue routines can be accessed via syscall instruction. The low level user library builds on it (basically
mq_send() and mq_recv() are just wrappers around these).
Unlike there, here're only three functions. The function is passed in %eax, and can be the following:

 - `'send'` for sending a message, see [isr_syscall()](https://github.com/bztsrc/osz/blob/master/src/core/syscall.c) and [src/core/(platform)/libk.S](https://github.com/bztsrc/osz/blob/master/src/core/x86_64/libk.S). Non blocking.
 - `'call'` sends a message and blocks until a response arrives.
 - `'recv'` returns a pointer to message queue to the oldest message. When the queue is empty and receiving is not possible, blocks.

The arguments are stored and read in System V ABI way: %rdi=pid_t thread, %rsi=event, %rdx=arg0/ptr, %rcx=arg1/size, %r8=arg2/magic.

Examples
```
    xorq    %rdi, %rdi
    movb    $SYS_sysinfo, %dil
    movl    $0x646E6573, %eax # 'send'
    syscall
```
or
```
    xorq    %rsi, %rsi
    movb    $EINVAL, %sil
    xorq    %rdi, %rdi
    movb    $SYS_seterr, %dil
    movl    $0x646E6573, %eax # 'send'
    syscall
```
Note that normally you never use these, you should use the mq_send() implementation in libc instead. These are only listed for
completeness.

