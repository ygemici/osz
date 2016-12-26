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
	int mq_start;
	int mq_end;
	int mq_size;
	pid_t mq_recvfrom;
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
use printf() and fopen() etc.

Low level user Library
----------------------

Only four functions, variations on synchronisation. They are provided by `libc`, and defined in [etc/include/syscall.h](https://github.com/bztsrc/osz/blob/master/etc/include/syscall.h).

```c
/* async, send a message (non-blocking, except dest queue is full) */
bool_t clsend(pid_t dst, uint64_t func, uint64_t arg0, uint64_t arg1, uint64_t arg2);
/* sync, send a message and receive result (blocking) */
msg_t *clcall(pid_t dst, uint64_t func, uint64_t arg0, uint64_t arg1, uint64_t arg2);
/* async, is there a message? (non-blocking) */
bool_t clismsg();
/* sync, wait until there's a message (blocking) */
msg_t *clrecv();
```

Supervisor Mode
---------------

Core can't use libc, it has it's own message queue implementation. Two functions in [src/core/msg.c](https://github.com/bztsrc/osz/blob/master/src/core/msg.c):

```c
bool_t msg_send(pid_t thread, uint64_t event, void *ptr, size_t size, uint64_t magic);
bool_t msg_sends(pid_t thread, uint64_t event, uint64_t arg0, uint64_t arg1, uint64_t arg2);
```

Those are variations on scalar / reference arguments and destnation translation. You can also
pass a system service number as pid, they can be found in `etc/include/syscall.h`.

If magic is given it can be a type hint on what's ptr is pointing to. It's optional as most events accept
only one kind of reference.

Low Level
---------

The lowest level of message sending can be found in [src/core/(platform)/libk.S](https://github.com/bztsrc/osz/blob/master/src/core/x86_64/libk.S) and it's a non-blocking call.

```c
void ksend(msghdr_t *mqhdr, uint64_t event, uint64_t arg0, uint64_t arg1, uint64_t arg2);
```

It's an effective assembly implementation on copying 32 bytes into the queue and handle start / end indeces.

Syscall ABI
-----------

There're only two functions, and their code is passed in %eax to syscall instruction. It can be

 - 'send' for sending a message, see isr_syscall() and ksend().
 - 'recv' is called when the queue is empty and receiving is not possible.

The arguments are stored and read in System V ABI way: %rdi=pid_t thread, %rsi=event, %rdx=arg0/ptr, %rcx=arg1/size, %r8=arg2/magic.
