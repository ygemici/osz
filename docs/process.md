OS/Z Processes and Tasks
========================

Object format
-------------

OS/Z uses ELF64 format to store executables on disk.

Processes
---------

Each process has a different [memory mapping](https://github.com/bztsrc/osz/tree/master/docs/memory.md).
They differ in their code segment, which holds their code and the required shared libraries. They differ in their bss segment
as well, only the shared memory is globally mapped.

Whenever the [scheduler](https://github.com/bztsrc/osz/blob/master/src/core/sched.c) chooses a task to execute that belongs to a different process than the process of the
interrupted task, the whole TLB cache is flushed and the whole memory mapping is reloaded.

Each process has at least one task. When the last task exists, the process is terminated.

Tasks
-----

Tasks of the same process share almost exactly the same memory mapping. They only differ in the first
page table, the Task Control Block and the [message queue](https://github.com/bztsrc/osz/blob/master/docs/messages.md).
Core has an optimized method for switching between tasks (when the text segment of the current and the new task are the same). This time only the first
pageslot (2M) is invalidated, the TLB cache is not flushed which gives much better performance.

This concept is closer to Solaris' Light Weight Processes than the well known pthread library where the
kernel does not know about threads at all. Note that in OS/Z pid_t points to a task, so userpace programs
see each task as a separate process, meaning real processes are totally transparent to them.

