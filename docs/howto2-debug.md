OS/Z - How To Series #2 - Debugging
===================================

Preface
-------

Last time we've checked how to [test OS/Z](https://github.com/bztsrc/osz/blob/master/docs/howto1-testing.md) in a virtual machine. In this episode we'll take a look on how to debug OS/Z.

First, you'll have to enable debugging in [Config](https://github.com/bztsrc/osz/blob/master/Config) by setting `DEBUG = 1`
and recompile.

Debug messages
--------------

If you want to see debug messages during boot, you can use the `debug` boot option in [FS0:\BOOTBOOT\CONFIG](https://github.com/bztsrc/osz/blob/master/etc/CONFIG). For available flags see [boot options](https://github.com/bztsrc/osz/blob/master/docs/bootopts.md).

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

If debugging enabled, boot will stop at the point where the system is about to release interrupts.
Hit <kbd>Enter</kbd> to start simulation.

Later you can press <kbd>Ctrl</kbd>+<kbd>Alt</kbd>+<kbd>Esc</kbd> inside the virtual machine to invoke
the internal debugger again.

<img align="left" style="padding-right:10px;width:300px;" src="https://github.com/bztsrc/osz/blob/master/docs/oszdbg2.png" alt="OS/Z Internar Debugger">
By default, debugger assumes serial console is a line oriented editor and printer. To enable video terminal mode, type
```
dbg> tui
```
You can get help any time by pressing <kbd>F1</kbd> either on keyboard or serial terminal, or by `help` command.

### Interfaces

By default the debugger is shown on framebuffer and accepts keyboard strokes. It also has a serial connection with 115200,8N1.
The later supports video terminal mode with `tui` command.
<img align="left" style="padding-right:10px;width:300px;" src="https://github.com/bztsrc/osz/blob/master/docs/oszdbg9.png" alt="OS/Z Internar Debugger Line Console">
<img align="left" style="padding-right:10px;width:300px;" src="https://github.com/bztsrc/osz/blob/master/docs/oszdbgA.png" alt="OS/Z Internar Debugger Text User Interface">

### Checking pid

It's on the right bottom corner of the screen.

### Panels (tabs)

| Name | Description |
| ---- | ----------- |
| Code | <img align="left" style="padding-right:10px;height:64px;" src="https://github.com/bztsrc/osz/blob/master/docs/oszdbg3.png" alt="OS/Z Internar Debugger Follow Execution">Register dump and code disassembly |
| Data | <img align="left" style="padding-right:10px;height:64px;" src="https://github.com/bztsrc/osz/blob/master/docs/oszdbg4.png" alt="OS/Z Internar Debugger Data Dump">Memory or stack dump |
| [Messages](https://github.com/bztsrc/osz/blob/master/docs/messages.md) | <img align="left" style="padding-right:10px;height:64px;" src="https://github.com/bztsrc/osz/blob/master/docs/oszdbg5.png" alt="OS/Z Internar Debugger Message Queue">Dump task's message queue |
| [TCB](https://github.com/bztsrc/osz/blob/master/src/core/x86_64/tcb.h)  | <img align="left" style="padding-right:10px;height:64px;" src="https://github.com/bztsrc/osz/blob/master/docs/oszdbg6.png" alt="OS/Z Internar Debugger Thread Control Block">Dump the current task's control block |
| [CCB](https://github.com/bztsrc/osz/blob/master/src/core/x86_64/ccb.h) | <img align="left" style="padding-right:10px;height:64px;" src="https://github.com/bztsrc/osz/blob/master/docs/oszdbg7.png" alt="OS/Z Internar Debugger CPU Control Block">Dump CPU Control Block (task priority queues) |
| [RAM](https://github.com/bztsrc/osz/blob/master/src/core/pmm.h) | <img align="left" style="padding-right:10px;height:64px;" src="https://github.com/bztsrc/osz/blob/master/docs/oszdbg8.png" alt="OS/Z Internar Debugger Physical Memory">Dump physical memory manager |

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
| Ctrl  | toggle instruction disassemble (bytes / mnemonics) |

### Debugger commands

It's enough to enter commands until it's obvious, in most cases that's the first letter only.

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
| `Tcb`      | examine current task's Thread Control Block |
| `TUi`      | toggle video terminal mode for serial console |
| `Messages` | examine message queue |
| `All, CCb`   | examine all queues and CPU Control Block |
| `Ram`      | examine RAM allocation, dump physical memory manager's data |
| `Instruction`, `Disasm` | instruction disassemble (or toggle bytes / mnemonics) |
| `Goto X`   | go to address X |
| `eXamine [/b1w2d4q8s] X`   | examine memory at X in byte, word, dword, qword or stack units |
| `Break [/b12d4q8rwx] X`   | list or set brakpoints at X in byte, word, dword, qword lengths |

Goto has an argument, entered in hexadecimal (prefix 0x is optional). Can be:
 * (empty) if not given, it will go to the next instruction
 * `+X`, `-X` set up rip relative address
 * `X` go to absolute address
 * `symbol+X` symbol relative address

Examine may have two arguments, a flag and an address. Address is the same as in goto, expect it sets rsp instead of rip.
 * (empty) return to the original stack
 * `+X`, `-X` set up rsp relative address
 * `X` go to absolute address
 * `symbol+X` symbol relative address

Break works just like examine, expect that
 * (empty) lists breakpoints
 * w in flag sets data write breakpoint (use /2 for word size)

The flag can be one of:
 * 1,b - 16 bytes in a row
 * 2,w - 8 words in a row
 * 4,d - 4 double words in a row
 * 8,q - 2 quad words in a row
 * s   - single quad word in a row (stack view, examine only)
 * r   - sets data read breakpoint (break only)
 * w   - sets data write breakpoint (break only)
 * p   - sets io port access breakpoint (break only)
 * x   - sets execution breakpoint (break only, default)

#### Examples

```
p               switch to previous thread
p 29            switch to thread that's pid is 29
g               go to next instruction (in view)
g isr_irq1+3    view instruction in function
g +7F           move disassembler window forward by 127 bytes
x 1234          dump memory at 0x1234
x /q            dump in quad words
x /s rsp        print stack
/b              switch units to bytes
i               go back to disassemble instructions tab
b /qw tcb+F0    set a write breakpoint for quadword length at 000F0h
b /bp 60        monitor keyboard port
```

The [next turorial](https://github.com/bztsrc/osz/blob/master/docs/howto3-rescueshell.md) is more user than developer oriented as it's about how to use the rescue shell.
