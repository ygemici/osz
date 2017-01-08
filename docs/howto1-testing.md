OS/Z - How To Series #1 - Testing
=================================

Preface
-------

In this episode we'll take a look on how to test a live image.

I always push the source to git in a state that's known to [compile](https://github.com/bztsrc/osz/tree/master/docs/compile.md) without errors by a `make clean all` command.
Although I did my best, it doesn't mean it won't crash under unforseen circumstances :-)

The [latest live dd image](https://github.com/bztsrc/osz/blob/master/bin/disk.dd?raw=true) should boot OS/Z in emulators and on real machines. For example type

```shell
qemu-system-x86_64 -hda bin/disk.dd
```

But for your convience I've added make commands. For example if you clone the repo and [compile](https://github.com/bztsrc/osz/blob/master/src/docs/compile.md), you can boot OS/Z right from your `bin/ESP` directory
with TianoCore EFI. For that one should type

```shell
make testefi
```

To run the resulting image in qemu with BIOS

```shell
make testq
```

To run the result with BIOS in bochs

```shell
make testb
```

To convert the raw disk image to something that VirtualBox can be fed with:

```shell
make vdi
```

First Break Point - Booting starts
----------------------------------

For this How To we are going to use bochs, as it would stop at predefined
addresses.

```sh
make testb
```

Our jurney begins before the firmware takes control of the hardware.
```
(0) [0x0000fffffff0] f000:fff0 (no symbol): jmpf 0xf000:e05b          ; ea5be000f0
<bochs:1> c
```

Just continue as we are not interested in the BIOS.

Second Break Point - OS/Z boot ends
-----------------------------------

The first intersting point is where the operating system was loaded, arranged
it's memory and interrupts and is about to leave privileged mode by executing the very first `iretq`.

You must see white on black "OS/Z ready." text on the top left corner of your screen,
and something similar to this on debug console:

```
(0) Magic breakpoint
Next at t=22187507
(0) [0x000000173c48] 0008:ffffffffffe0ac48 (sys_enable+83): iret                      ; 48cf
<bochs:2>
```

This sys_enable() function is in [src/core/(platform)/sys.c](https://github.com/bztsrc/osz/blob/master/src/core/x86_64/sys.c) and it's
job is to map the first task and start executing it.


As the stack stores user mode segment selectors, the CPU will drop
privileged mode, and normal user space execution began.
```
<bochs:2> print-stack
Stack address size 8
 | STACK 0x0000000000000fd8 [0x00000000:0x00200000]
 | STACK 0x0000000000000fe0 [0x00000000:0x00000023]
 | STACK 0x0000000000000fe8 [0x00000000:0x00003002]
 | STACK 0x0000000000000ff0 [0x00000000:0x001fffa0]
 | STACK 0x0000000000000ff8 [0x00000000:0x0000001b]
<bochs:3> s

```
We can see that the user mode code starts at 0x200000, and it's stack is right beneath it (which differs from IRQ handler's safe stack).

By stepping through the iret instruction with <kbd>s</kbd>, you'll find yourself on the [.text.user](https://github.com/bztsrc/osz/blob/master/src/core/x86_64/user.S) segment of core. This code
will iterate through detected devices by calling their _init() method.

Third Break Point - Enabling Multitask
--------------------------------------

At the third break point we can see that driver initialization finished, and "SYS" task is
about to send a message to core to enable interrupts. By doing so, it will unleash hell, as nobody
will know for sure in which order the interrupts fire. Luckily the message queue is serialized, so there's
no need for locking.

```
(0) Magic breakpoint
Next at t=22686813
(0) [0x00000018104c] 0023:000000000020004c (_init+4c): mov eax, 0x646e6573       ; b873656e64 'send'
<bochs:8>
```

You can count on me when I say we're still on "SYS" task (if in doubt type `x /5bc 0`). So load
it's symbols, and check code.

```
<bochs:8> ldsym global "bin/sys.sym"
<bochs:9> u /8 rip
0020004c: (            _init+4c): mov eax, 0x646e6573       ; b873656e64 'send'
00200051: (            _init+51): syscall                   ; 0f05
00200053: (           getwork+0): call .+153                ; e899000000 call mq_recv
00200058: (           getwork+5): cmp word ptr ds:[rax], 0x0000 ; 66833800
0020005c: (           getwork+9): jnz .-11                  ; 75f5
0020005e: (           getwork+b): mov rdi, qword ptr ds:[rax+8] ; 488b7808
00200062: (           getwork+f): mov rsi, 0x0000000000201000 ; 48c7c600102000
00200069: (          getwork+16): xor rcx, rcx              ; 4831c9
<bochs:10>
```

The execution stopped right before the kernel call. After that comes [getwork](https://github.com/bztsrc/osz/blob/master/src/core/x86_64/user.S) function that calls mq_recv()
to get some work. When there's no more messages left, it blocks and scheduler picks the next
task to run. The OS will switch to the next task every time an interrupt fires or the current task blocks.

Further Break Points
--------------------

No more breakpoints (no more planned break points that is).
Press <kbd>Ctrl</kbd>+<kbd>C</kbd> to interrupt and get to bochs console any time you want.

If you are interested in debugging, read the [next turorial](https://github.com/bztsrc/osz/blob/master/docs/howto2-debug.md).
