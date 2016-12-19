OS/Z Memory Management
======================

Memory is allocated at several levels:
 - pmm_alloc() allocates physical RAM pages
 - kalloc() allocates core (kernel) bss memory
 - malloc(), realloc(), calloc(), free() allocate user space bss memory
 - smalloc(), srealloc(), scalloc(), sfree() allocate shared memory

Memory Mapping
--------------

All processes have their own memory mappings. Some pages are different,
some are shared. There's a special process, called "system" which has
a slightly different mapping than normal processes.

System process
--------------

| Virtual Address | Scope | Description |
| --------------- | ----- | ----------- |
| -2^56 .. -4G-1  | global  | global shared memory space (user accessible) |
| -4G .. 0        | global  | core memory (supervisor only) |
| 0 .. 4096       | thread  | Thread Control Block |
| 4096 .. x       | thread  | message queue |
| x  .. 2M-1      | thread  | local stack, growing downwards |
| 2M .. x         | [process](https://github.com/bztsrc/osz/tree/master/docs/process.md)  | message queue dispatcher |
| x .. 4G-1       | process | device drivers (shared objects) |
| 4G .. 2^56      | process | dynamically allocated driver memory and MMIO mappings, growing upwards |

The system process is the only one that allowed to execute in/out instructions, and it maps MMIO areas as user writable
pages in it's bss segment. Each device in the system should have a device driver loaded in the system process.
When an IRQ occurs, the Core sends a message to the system process and it's dispatcher calls the irq handler in the
appropriate shared library.

Also the system process is counted for the idle task.

User processes
--------------

| Virtual Address | Scope | Description |
| --------------- | ----- | ----------- |
| -2^56 .. -4G-1  | global  | global shared memory space (user accessible) |
| -4G .. 0        | global  | core memory (supervisor only) |
| 0 .. 4096       | thread  | Thread Control Block |
| 4096 .. x       | thread  | message queue |
| x  .. 2M-1      | thread  | local stack, growing downwards |
| 2M .. x         | [process](https://github.com/bztsrc/osz/tree/master/docs/process.md)  | user program text segment |
| x .. 4G-1       | process | shared libraries |
| 4G .. 2^56      | process | dynamically allocated bss memory, growing upwards |

Normal userspace processes do not have any MMIO, only physical RAM can be mapped in their bss segment.
If two mappings are identical save the TCB and message queue, their threads belong to the same process.
