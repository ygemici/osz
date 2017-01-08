OS/Z - How To Series #2 - Debugging
===================================

Preface
-------

Last time we've checked how to [test OS/Z](https://github.com/bztsrc/osz/blob/master/docs/howto1-testing.md) in a virtual machine. In this episode we'll take a look on how to debug OS/Z.

Debugging with GDB
------------------

First, you'll have to enable debugging in [Config](https://github.com/bztsrc/osz/blob/master/Config) by setting `DEBUG = 1`. Then

```sh
make debug
```

and then in another terminal, type

```sh
make gdb
```

to start the debugger and connect it to the running OS/Z instance. Type <kbd>c</kbd> and <kbd>Enter</kbd> to start simulation.

Debugging with bochs
--------------------

Run with

```sh
make testb
```

Type <kbd>c</kbd> and <kbd>Enter</kbd> to start simulation. Later on press <kbd>Ctrl</kbd>+<kbd>C</kbd> to get to bochs debug console.

### Get pid

To obtain your current pid, you'll type simply `page 0`.

<pre>
&lt;bochs:2> page 0
PML4: 0x0000000000033007    ps         a pcd pwt U W P
PDPE: 0x0000000000034007    ps         a pcd pwt U W P
 PDE: 0x8000000000035007 XD ps         a pcd pwt U W P
 PTE: 0x8000000000031005 XD    g pat d a pcd pwt U R P
linear page 0x0000000000000000 maps to physical page 0x<i><b>000000031</b></i>000
&lt;bochs:3>
</pre>

And look for the last number on output. Forget the last three zeros, and there's your pid. It's `0x31` in our case.

### Checking pid

You can check the validity of a pid anytime with:

```
<bochs:3> xp /5bc 0x31000
[bochs]:
0x0000000000031000 <bogus+       0>:  T    H    R    D   \0
<bochs:4>
```

Here we can see that the page starts with the magic `'THRD'` so identifies as a Thread Control Block. The
number tells us that it's priority. In our case it's `priority queue 0`, meaning it's "SYS" task we're watching.

Save the first 8 bytes, the actual bitfields of this TCB page is platform specific as it's holding a copy of the CPU state as well.
Each struct definition can be found in the according platform's directory [src/core/(platform)/tcb.h](https://github.com/bztsrc/osz/blob/master/src/core/x86_64/tcb.h).

For current task, use `x /5bc 0`.

Debugging with Internal Debugger
--------------------------------

If debugging enabled, press <kbd>Ctrl</kbd>+<kbd>Alt</kbd>+<kbd>Esc</kbd> to invoke debugger.

### Panels (tabs)

| Name | Description |
| ---- | ----------- |
| Code | Register dump and code disassembly |
| [TCB](https://github.com/bztsrc/osz/blob/master/src/core/x86_64/tcb.h)  | Dump the current task's control block |
| [Messages](https://github.com/bztsrc/osz/blob/master/docs/messages.md) | Dump task's message queue |
| [CCB](https://github.com/bztsrc/osz/blob/master/src/core/x86_64/ccb.h) | Dump CPU COntrol Block (task queues) |
| [RAM](https://github.com/bztsrc/osz/blob/master/src/core/pmm.h) | Dump physical memory manager |

### Keyboard Shortcuts

| Key   | Description |
| ----- | ----------- |
| F1    | displays help |
| Esc   | exit debugger (continue or reboot in case of panic) |
| Tab   | switch panels |
| Enter | (with empty command) step instruction |
| Enter | (with command) execute command |
| Left  | (with empty command) switch to previous task |
| Left  | (with command) move cursor |
| Right | (with empty command) switch to next task |
| Right | (with command) move cursor |
| Ctrl  | toggle instruction disassemble (bytes / mnemonics) |

### Debugger commands

It's enough to enter commands until it's obvious, in most cases that's the first letter only.

| Command  | Description |
| -------- | ----------- |
| `step`     | step instruction |
| `continue` | continue execution |
| `reset`, `reboot` | reboot computer |
| `halt`     | power off computer |
| `help`     | displays help |
| `prev`     | switch to previous task |
| `next`     | switch to next task |
| `tcb`      | examine current task's Thread Control Block |
| `messages` | examine message queue |
| `queues`   | examine task queues |
| `ram`      | examine RAM allocation |
| `instruction` | toggle instruction disassemble (bytes / mnemonics) |
| `goto X`   | go to address X |

Goto has an argument, entered in hexadecimal (prefix 0x is optional). Can be:
 * (empty) if not given, it will go to the next instruction
 * `rip` return to the original instruction
 * `+X`, `-X` set up rip relative address
 * `X` go to absolute address

The [next turorial](https://github.com/bztsrc/osz/blob/master/docs/howto3-rescueshell.md) is more user friendly as it's about
how to use the rescue shell.
