OS/Z Message Queues
===================

The main inter process communication form of OS/Z tasks are message queues.

User level library
------------------

Normally you won't see a thing about message queues. The `libc` library hides all the details from you, so you just
use printf() and fopen() etc. as usual. For [security](https://github.com/bztsrc/osz/blob/master/docs/security.md) reasons, you cannot use message queues directly, and you should
not use the syscall interface. The `libc` functions guarantee the [portability](https://github.com/bztsrc/osz/blob/master/docs/porting.md) of applications.

But if there's really a need to use the messaging system (you are implementing a new message protocol), then you have platform
independent `libc` functions for that.

### Low level user Library

There are only five functions for messaging, variations on synchronisation. They are provided by `libc`, and defined in [etc/include/sys/core.h](https://github.com/bztsrc/osz/blob/master/etc/include/sys/core.h).
You should not use them directly (unless you're implementing your own message protocol), use one of the higher level functions instead.

```c
/* async, send a message and return it's serial (non-blocking) */
uint64_t mq_send(pid_t dst, uint64_t func, ...);

/* sync, send a message and receive result (blocking) */
msg_t *mq_call(pid_t dst, uint64_t func, ...);

/* async, is there a message? returns serial or 0 (non-blocking) */
uint64_t mq_ismsg();

/* sync, wait until there's a message (blocking) */
msg_t *mq_recv();

/* sync, dispatch events (blocking, noreturn) */
void mq_dispatch();
```
While [mq_call()](https://github.com/bztsrc/osz/blob/master/src/lib/libc/x86_64/syscall.S) is a sequence of mq_send() and my_recv() calls, [mq_dispatch()](https://github.com/bztsrc/osz/blob/master/src/lib/libc/dispatch.c) is quite the opposite: it first
receives a message with mq_recv(), calls the handler for it, and then uses mq_send() to send the result back. For msg_t struct, see below.

You can also pass a system service number as `dst`, they can be found in [etc/include/syscall.h](https://github.com/bztsrc/osz/blob/master/etc/include/syscall.h). The available function codes
are defined in the corresponding header file under [etc/include/sys](https://github.com/bztsrc/osz/blob/master/etc/include/sys), all included in this header.

Supervisor Mode (Ring 0)
------------------------

Core can't use libc, it has it's own message queue implementation. Three functions in [src/core/msg.c](https://github.com/bztsrc/osz/blob/master/src/core/msg.c):

```c
bool_t msg_send(
    evt_t event,
    void *ptr,
    size_t size,
    uint64_t magic);

bool_t msg_sends(
    evt_t event,
    uint64_t arg0,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3,
    uint64_t arg4,
    uint64_t arg5);

bool_t msg_syscall(
    evt_t event,
    uint64_t arg0,
    uint64_t arg1,
    uint64_t arg2);
)
```

Those are variations on scalar and reference arguments, as `core` never blocks, so they are all async.
Also they use (dst<<16 | func) as event.

If magic is given it can be a type hint on what's ptr is pointing to. It's optional as most events accept
only one kind of reference.

The third version handles messages sent specifically to the `core` (like SYS_exit or SYS_alarm events) efficiently.

### Low Level

The platfrom specific lowest level of message sending can be found in [src/core/(platform)/libk.S](https://github.com/bztsrc/osz/blob/master/src/core/x86_64/libk.S) and it's an atomic, non-blocking call.

```c
void ksend(
    msghdr_t *mqhdr,
    uint64_t func,
    uint64_t arg0,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3,
    uint64_t arg4,
    uint64_t arg5);
```

It's an efficient assembly implementation of copying msg_t into the queue and handle start / end indeces. The `msg_sends()`
function calls it after it had mapped the destination's task message queue temporarily.

From userspace the message queue routines can be accessed via `syscall` instruction on x86_64 platform. The low level user library
builds on it (basically mq_send() and mq_recv() and all the other libc functions are just wrappers).
The destination and function is passed in %rax. When destination is SRV_CORE (0) and function is SYS_recv (0x7FFF), it receives
messages. Any other value sends.

The arguments are stored and read in System V ABI way, with one exception: %rcx is clobbered by the syscall instruction, so
it must be passed in %rbx.
```
%rax= (pid_t task) << 16 | function,
%rdi= arg0/ptr
%rsi= arg1/size
%rdx= arg2/magic
%rbx= arg3
%r8 = arg4
%r9 = arg5
syscall
```
Note that you must never use this syscall instruction from your code, you have to use `mq_send()`, `mq_call()` and the others instead.
Or even better, use higher level functions in `libc`.

Structures
----------

It's a so essential type that it's defined in [etc/include/sys/types.h](https://github.com/bztsrc/osz/blob/master/etc/include/sys/types.h#L117).

```c
msg_t *mq;
```

The first item in the array is the queue header. Because the exact implementation of msghdr_t is irrelevant to the queue's user, it scope is core only and defined
in [src/core/msg.h](https://github.com/bztsrc/osz/blob/master/src/core/msg.h).

```c
msghdr = (msghdr_t *)mq[0];
```

The other items (starting from 1) form a circular buffer for a FIFO queue. The size of that buffer is `msghdr_t.mq_size - 1`.

