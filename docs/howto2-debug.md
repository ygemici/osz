OS/Z - How To Series #2 - Debugging
===================================

Preface
-------

Last time we've checked how to [test OS/Z](https://github.com/bztsrc/osz/blob/master/docs/howto1-testing.md) in a virtual machine. In this episode we'll take a look on how to debug OS/Z. With the serial console, it's easy to connect any RS-232 cable and you're ready to debug on real hardware.

But first of all, you'll have to enable debugging in [Config](https://github.com/bztsrc/osz/blob/master/Config) by setting `DEBUG = 1`
and recompile.

Debug messages
--------------

If you want to see debug messages during boot, you can use the `debug` boot option in [environment](https://github.com/bztsrc/osz/blob/master/etc/sys/config). For available flags see [boot options](https://github.com/bztsrc/osz/blob/master/docs/bootopts.md).
It is not really necessary as with `DEBUG = 1` the syslog is also sent to the serial line, and it has all the necessary information.

Debugging with GDB
------------------

Start a virtual machine in standby mode with

```sh
make debug
```

and then in another terminal, type

```sh
make gdb
```

to start [GDB](https://www.sourceware.org/gdb/) and connect it to the running OS/Z instance. Type <kbd>c</kbd> and <kbd>Enter</kbd> to start simulation.

### Get pid

Not possible, at least there's no way I know of.

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
0x0000000000031000 <bogus+       0>:  T    A    S    K   \2
<bochs:4>
```

Here we can see that the page starts with the magic `'TASK'` so identifies as a Task Control Block. The
number tells us the task's priority, in our case `priority queue 2`.

Save the first 8 bytes, the actual bitfields of this TCB page is platform specific as it's holding a copy of the CPU state as well.
Each struct definition can be found in the according platform's directory [src/core/(platform)/tcb.h](https://github.com/bztsrc/osz/blob/master/src/core/x86_64/tcb.h).

For current task, use `x /5bc 0`.

Debugging with Internal Debugger
--------------------------------

<img align="left" style="padding-right:10px;" height="64" src="https://github.com/bztsrc/osz/blob/master/docs/oszdbg2.png?raw=true" alt="OS/Z Internal Debugger">
You can press <kbd>Ctrl</kbd>+<kbd>Alt</kbd>+<kbd>Esc</kbd> inside the virtual machine or send a <kbd>Break</kbd> through
serial line to invoke the internal debugger.

You can invoke it from your code at any given point as well with

```c
/* insert a breakpoint into C code */
breakpoint;
/* insert a bochs breakpoint into C code */
breakbochs;
/* insert a gdb breakpoint into C code. In gdb use "set $pc+=2" */
breakgdb;
```

### Interfaces

The internal debugger is shown on framebuffer and accepts keyboard strokes.

It also has a serial connection with 115200,8N1. By default assumes serial console is a line oriented editor and printer. To enable video terminal mode, type
```
dbg> tui
```


<img align="left" style="padding-right:10px;" height="180" src="https://github.com/bztsrc/osz/blob/master/docs/oszdbg3.png?raw=true" alt="OS/Z Internal Debugger" title="OS/Z Internal Debugger">
<img align="left" style="padding-right:10px;" height="180" src="https://github.com/bztsrc/osz/blob/master/docs/oszdbg9.png?raw=true" alt="OS/Z Internal Debugger Line Console" title="OS/Z Internal Debugger Line Console">
<img height="180" src="https://github.com/bztsrc/osz/blob/master/docs/oszdbgA.png?raw=true" alt="OS/Z Internal Debugger Text User Interface" title="OS/Z Internal Debugger Text User Interface">

You can get help any time by pressing <kbd>F1</kbd> either on keyboard or serial terminal, or by typing `help`.

### Checking pid

It's on the right bottom corner of the screen.

In line console mode, it's shown before the `dbg>` prompt.

Within the debugger, use `dbg> p pid` or <kbd>&larr;</kbd> and <kbd>&rarr;</kbd> keys to switch tasks.

### Panels (tabs)

| Name | Description |
| ---- | ----------- |
| Code | <img align="left" style="padding-right:10px;" height="64" src="https://github.com/bztsrc/osz/blob/master/docs/oszdbg3.png?raw=true" alt="OS/Z Internal Debugger Follow Execution">Register dump and code disassembly |
| Data | <img align="left" style="padding-right:10px;" height="64" src="https://github.com/bztsrc/osz/blob/master/docs/oszdbg4.png?raw=true" alt="OS/Z Internal Debugger Data Dump">Memory or stack dump |
| [Messages](https://github.com/bztsrc/osz/blob/master/docs/messages.md) | <img align="left" style="padding-right:10px;" height="64" src="https://github.com/bztsrc/osz/blob/master/docs/oszdbg5.png?raw=true" alt="OS/Z Internal Debugger Message Queue">Dump task's message queue |
| [TCB](https://github.com/bztsrc/osz/blob/master/src/core/x86_64/tcb.h)  | <img align="left" style="padding-right:10px;" height="64" src="https://github.com/bztsrc/osz/blob/master/docs/oszdbg6.png?raw=true" alt="OS/Z Internal Debugger Task Control Block">Dump the current task's control block |
| [CCB](https://github.com/bztsrc/osz/blob/master/src/core/x86_64/ccb.h) | <img align="left" style="padding-right:10px" height="64" src="https://github.com/bztsrc/osz/blob/master/docs/oszdbg7.png?raw=true" alt="OS/Z Internal Debugger CPU Control Block">Dump CPU Control Block (task priority queues) |
| [RAM](https://github.com/bztsrc/osz/blob/master/src/core/pmm.h) | <img align="left" style="padding-right:10px;" height="64" src="https://github.com/bztsrc/osz/blob/master/docs/oszdbg8.png?raw=true" alt="OS/Z Internal Debugger Physical Memory">Dump physical memory manager |
| [Sysinfo](https://github.com/bztsrc/osz/blob/master/src/core/syslog.c) | <img align="left" style="padding-right:10px;" height="64" src="https://github.com/bztsrc/osz/blob/master/docs/oszdbgB.png?raw=true" alt="OS/Z Internal Debugger Sysinfo">System Information and early syslog buffer |

### Keyboard Shortcuts

| Key   | Description |
| ----- | ----------- |
| F1    | displays help |
| Esc   | (with empty command) exit debugger, continue or reboot in case of panic |
| Esc   | (with command) clear command |
| Tab   | switch panels |
| Enter | (with empty command) step instruction |
| Enter | (with command) execute command |
| Left  | (with empty command) switch to previous task |
| Left  | (with command) move cursor |
| Right | (with empty command) switch to next task |
| Right | (with command) move cursor |
| Up    | previous command in history |
| Up    | with <kbd>Shift</kbd>, scroll up |
| Down  | next command in history |
| Down  | with <kbd>Shift</kbd>, scroll down |
| PgUp  | in data view, scroll one page up |
| PgUp  | in data view with <kbd>Shift</kbd>, scroll one slot up |
| PgDn  | in data view, scroll one page down |
| PgDn  | in data view with <kbd>Shift</kbd>, scroll one slot down |
| Space | toggle instruction disassemble (bytes / mnemonics) |

### Debugger commands

It's enough to enter commands until they became obvious, in most cases that's the first letter only. I've used uppercase letters
to indicate that.

| Command  | Description |
| -------- | ----------- |
| `Step`     | step instruction |
| `Continue` | continue execution |
| `REset`, `REboot` | reboot computer |
| `HAlt`     | power off computer |
| `Help`     | displays help |
| `Pid X`    | switch to task |
| `Prev`     | switch to previous task |
| `Next`     | switch to next task |
| `Tcb`      | examine current task's Task Control Block |
| `TUi`      | toggle video terminal mode for serial console |
| `Messages` | examine message queue |
| `All, CCb`   | examine all queues and CPU Control Block |
| `Ram`      | examine RAM allocation, dump physical memory manager's data |
| `Instruction`, `Disasm` | instruction disassemble (or toggle bytes / mnemonics) |
| `Goto X`   | go to address X |
| `eXamine [/b1w2d4q8s] X`   | examine memory at X in byte, word, dword, qword or stack units |
| `Break [/b12d4q8rwx] X`   | list or set brakpoints at X in byte, word, dword, qword lengths |
| `SYsinfo` `Log` | show system information and early syslog |
| `Full`     | toggle full window mode, give space for main information |

Goto has an argument, entered in hexadecimal (prefix 0x is optional). Note this only moves the window, but does
not change execution flow. Can be:
 * (empty) if not given, it will go to the next instruction
 * `+X`, `-X` set up rip relative address
 * `X` go to absolute address
 * `symbol+X` symbol relative address

Examine may have two arguments, a flag and an address. Address is the same as in goto, expect it sets rsp.
 * (empty) return to the original stack
 * `+X`, `-X` set up rsp relative address
 * `X` go to absolute address
 * `symbol+X` symbol relative address

Exampine has a counterpart, `xp` which examines physical addresses.

Break works just like examine, expect that
 * (empty) lists breakpoints
 * w in flag sets data write breakpoint (use /2 for word size)

The flag can be one of:
 * 1,b - 16 bytes in a row
 * 2,w - 8 words in a row
 * 4,d - 4 double words in a row
 * 8,q - 2 quad words in a row
 * s   - single quad word in a row (stack view with symbol translation, examine only)
 * r   - sets data read breakpoint (break only)
 * w   - sets data write breakpoint (break only)
 * p   - sets io port access breakpoint (break only)
 * x   - sets execution breakpoint (break only, default)

In addition to ELF symbols, you can also use these:
 * tcb   - task control block
 * mq    - message queue
 * stack - start of local stack
 * text  - start of text segment, end of local stack
 * bss   - dynamically allocated memory
 * sbss/shared - shared memory
 * core  - kernel text segment
 * buff  - buffer area
 * bt    - backtrace stack
 * cr2, cr3 +any general purpose register

#### Examples

```
p               switch to previous task
p 29            switch to task that's pid is 29
g               go to next instruction (in view, do not confuse with single step)
g isr_irq1+3    view instruction in function
g +7F           move disassembler window forward by 127 bytes
x 1234          dump memory at 0x1234
x /q            dump in quad words
x /s rsp        print stack
/b              switch units to bytes
x /4 rsi        show dwords at register offset
i               go back to disassemble instructions tab
b /qw tcb+F0    set a write breakpoint for quadword length at 000F0h
b /bp 60        monitor keyboard port
s               single step one instruction
sy              show sysinfo
f               toggle full window mode, show only logs on sysinfo tab
```

The [next turorial](https://github.com/bztsrc/osz/blob/master/docs/howto3-develop.md) is about how write device drivers and applications for OS/Z.
