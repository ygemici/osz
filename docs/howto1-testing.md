OS/Z - How To Series #1 - Testing
=================================

Preface
-------

In this episode we'll take a look on how to test a live image.

I always push the source to git in a state that's known to [compile](https://github.com/bztsrc/osz/tree/master/docs/compile.md) without errors by a `make clean all` command.
Although I did my best, it doesn't mean it won't crash under unforseen circumstances :-)

The [latest live dd image](https://github.com/bztsrc/osz/blob/master/bin/) should boot OS/Z in emulators and on real machines. For example type

```shell
qemu-system-x86_64 -hda bin/osZ-latest-x86_64-ibmpc.dd
```

But for your convience I've added make commands. For example if you clone the repo and [compile](https://github.com/bztsrc/osz/blob/master/docs/compile.md), you can boot OS/Z right from your `bin/ESP` directory
with TianoCore EFI. For that one should type

```shell
make testesp
```

To run the resulting image in qemu with TianoCore UEFI

```shell
make teste
```

To run the resulting image in qemu with BIOS

```shell
make testq
```

To run the result with BIOS in bochs

```shell
make testb
```

To run in VirtualBox (either BIOS or UEFI depending on your configuration):

```shell
make testv
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
Next at t=0
(0) [0x0000fffffff0] f000:fff0 (no symbol): jmpf 0xf000:e05b          ; ea5be000f0
<bochs:1> c
```

Just continue as we are not interested in the BIOS.

Second Break Point - OS/Z boot ends
-----------------------------------

<img align="left" style="margin-right:10px;" width="300" src="https://github.com/bztsrc/osz/blob/master/docs/oszdbg0.png?raw=true" alt="OS/Z Starting">

The first intersting point is where the operating system was loaded (arranged it's memory, finished with task setup)
and is about to leave privileged mode by executing the very first `iretq`.

You must see white on black "OS/Z starting..." text on the top left corner of your screen,
and something similar to this on debug console:

```
(0) Magic breakpoint
Next at t=45593694
(0) [0x00000019b263] 0008:ffffffffffe15263 (sys_enable+be): iret                      ; 48cf
<bochs:2> 
```

This `sys_enable()` function is in [src/core/(platform)/sys.c](https://github.com/bztsrc/osz/blob/master/src/core/x86_64/sys.c) and it's
job is to map the first task and start executing it by faking a return from ISR.

As the stack stores user mode segment selectors, the CPU will drop
privileged mode, and normal user space (ring 3) execution will began.
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

Third Break Point - OS/Z ready
------------------------------

<img align="left" style="margin-right:10px;" width="300" src="https://github.com/bztsrc/osz/blob/master/docs/oszdbg1.png?raw=true" alt="OS/Z ready">

When all drivers and system tasks initialized and blocked waiting for messages, a function named `sys_ready()` is called.
That will in turn call `isr_fini()` which will finish ISR initialization by enabling IRQs with registered tasks.

```
(0) Magic breakpoint
Next at t=45857039
(0) [0x00000019b616] 0008:ffffffffffe15616 (sys_ready+131): mov eax, 0x00000000       ; b800000000
<bochs:3>
```

By doing so, it will enable pre-emption and unleash hell, as nobody will know for sure in which order the interrupts fire.
Luckily the message queue is serialized, so there's no need for locking.

<img align="left" style="margin-right:10px;" width="80" src="https://github.com/bztsrc/osz/blob/master/docs/oszdbg2.png?raw=true" alt="OS/Z Debugger">

Please note that OS/Z has it's own internal debugger, so don't have to use bochs debugger if you don't want to.
Here's a screenshot as a teaser. It has a nice interface and available on serial line too (VT100 mode supported).

If you are interested in debugging, read the [next tutorial](https://github.com/bztsrc/osz/blob/master/docs/howto2-debug.md).
