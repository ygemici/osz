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

