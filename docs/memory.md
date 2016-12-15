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
| 4096 .. 2M      | thread  | message queue |
| 2M .. 4G        | [process](https://github.com/bztsrc/osz/tree/master/docs/process.md)  | message queue dispatcher |
| 4G .. 16G-1     | process | device drivers (shared objects) |
| 16G .. x        | process | dynamically allocated driver memory and MMIO mappings, growing upwards |
| x  .. 2^56      | thread  | thread local stack, growing downwards |

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
| 4096 .. 2M      | thread  | message queue |
| 2M .. 4G-1      | [process](https://github.com/bztsrc/osz/tree/master/docs/process.md)  | user program text segment |
| 4G .. 16G-1     | process | shared libraries |
| 16G .. x        | process | dynamically allocated bss memory, growing upwards |
| x  .. 2^56      | thread  | thread local stack, growing downwards |

Normal userspace processes do not have any MMIO, only physical RAM can be mapped in their bss segment.
If two mappings are identical save the TCB and message queue, their threads belong to the same process.
