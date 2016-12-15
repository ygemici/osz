OS/Z Processes and Threads
==========================

Object format
-------------

OS/Z uses ELF64 format to store executables on disk. For shared libraries it adds a new section named ".cl",
for storing additional linkage information. This is required for the dynamic linker to handle overloaded
functions. Unlike standard C, OS/Z allows and builds on multiple dispatch.

Processes
---------

Each process has a different [memory mapping](https://github.com/bztsrc/osz/tree/master/docs/memory.md).
They differ in their code segment, which holds their code and the required shared libraries. They differ in their bss segment
as well, only the shared memory is globally mapped.

Whenever the Core chooses a thread to execute that belongs to a different process than the process of the
interrupted thread, the whole TLB cache is flushed and the whole memory mapping is reloaded.

Each process has at least one thread. When the last thread exists, the process is terminated.

Threads
-------

Threads of the same process share almost exactly the same memory mapping. They only differ in the first
page table, the Thread Control Block and the message queue. Core has an optimized method for switching between
threads (when the text segment of the current and the new thread are the same). This time only the first
pagetable (2M, TCB and the message queue) is invalidated, the TLB cache is not flushed which gives much better
performance.

This concept is closer to Solaris' Light Weight Processes than the well known pthread library where the
kernel does not know about threads at all. Note that in OS/Z pid_t points to a thread, so userpace programs
see each thread as a separate process, meaning processes are totally transparent to them.

Scheduling
----------

An operating system should guarantee that no task will starve ever. Linux try to solve this with dynamically
calculating a number to add to priority. OS/Z has a fundamentally different approach, namely re-entrant priority
levels. This means that one time slice at each priority level is reserved for a lower priority task.

For example if we have 3 threads at priority 5, and no higher priority tasks are allowed to run, then the
time will be splitted into 4. Out of that 4 time slices 3 are for threads at level 5, and one slice
is for a thread at level 6 or below. Within a given priority level the thread is choosen in round-rubin
style, but threads of the same process are grouped together thus minimalizing the number of TLB cache flushes.

Priority levels
---------------

OS/Z has 8 priority levels:

| 0 | PRI_SYS | System |
| 1 | PRI_RT | Real Time tasks |
| 2 | PRI_DRV | Userspace drivers |
| 3 | PRI_SRV | Services |
| 4 | PRI_APPH | Application High |
| 5 | PRI_APP | Application Normal |
| 6 | PRI_APPL | Application Low |
| 7 | PRI_IDLE | Idle |

The top priority level is PRI_SYS (zero). Only one process called "system" allowed to run at this level, and
this level is uninterruptible meaning there's no one slice time for a lower priority task.

After that comes PRI_RT (1) for real time processes.

That's followed by PRI_DRV (2) for user space device and filesystem drivers.

The next one is PRI_SRV (3) for services aka unix daemons.

Normal applications have three levels, which allows balancing different programs. These are PRI_APPH (4, high priority)
for example video players and games, PRI_APP (5, normal priority) for example editors and browsers, and PRI_APPL
(6, low priority) for example some background tasks.

Finally PRI_IDLE (7). The last level just like the first one is special. Normally this level is never reached, only
if there're no other runnable (non-blocked) threads at higher priority levels. The screensaver and memory defragmenter
runs at this level for example. If even this level is empty or blocked, then a special function is scheduled in the
"system" process that halts the CPU.

Example
-------

Imagine the following threads:

 - priority level 4: A, B, C
 - priority level 5: D
 - priority level 6: E, F

They will be scheduled in order:

 A B C D A B C E A B C D A B C F

All threads had the chance to run, so there was no stravation and time was splitted up as follows:

 A: 4 times
 B: 4 times
 C: 4 times
 D: 2 times
 E: 1 time
 F: 1 time
 
Which is pretty much the distribution one would expect for those priority levels.
