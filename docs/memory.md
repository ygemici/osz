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
some are shared. There's a special task, called "system" which has
a slightly different mapping than normal user tasks.

System Task
-----------

| Virtual Address | Scope | Description |
| --------------- | ----- | ----------- |
| -2^56 .. -1G-1  | global  | global shared memory space (user accessible, read/write) |
|   -1G .. 0      | global  | core memory (supervisor only) |
|     0 .. 4096   | thread  | Thread Control Block |
|  4096 .. 1M-1   | thread  | Message Queue |
|    1M .. x      | thread  | temporarily mapped message buffer (growing upwards) |
|     x .. 2M-1   | thread  | local stack (growing downwards) |
|    2M .. 2M+1p-1| [process](https://github.com/bztsrc/osz/tree/master/docs/process.md)  | message queue dispatcher, loaded from .text.user section |
| 2M+1p .. 2M+1p-1| process | IRQ Dispatch Table |
| 2M+2p .. 4G-1   | process | device drivers (shared objects) |
|    4G .. 2^56   | process | dynamically allocated driver memory and MMIO mappings, growing upwards |

The system task is the only one that allowed to execute in/out instructions, and it maps MMIO areas as user writable
pages in it's bss segment. Each device in the system should have a device driver loaded in the system process.
When an IRQ occurs, the Core sends a message to the system task and it's dispatcher calls the irq handler in the
appropriate shared library.

Also the system task is accounted for the idle task.

IRQ Dispatch Table is an array of entry points, save the first item, which is the maximum number of handlers
per IRQ (idt[0]). The list of functions to call on IRQ x is at idt[x * idt[0] + 1] and idt[(x+1) * idt[0]] inclusive. The table
is filled up at boot time when the device [drivers](https://github.com/bztsrc/osz/tree/master/docs/drivers.md) are detected.
The variable idt[0] can be set in [etc/CONFIG](https://github.com/bztsrc/osz/tree/master/etc/CONFIG) with "nrirqmax" variable.

User Tasks
----------

| Virtual Address | Scope | Description |
| --------------- | ----- | ----------- |
| -2^56 .. -1G-1  | global  | global shared memory space (user accessible, read/write) |
|   -1G .. 0      | global  | core memory (supervisor only) |
|     0 .. 4096   | thread  | Thread Control Block (read-only) |
|  4096 .. 1M-1   | thread  | Message Queue (read/write) |
|    1M .. x      | thread  | temporarily mapped message buffer (growing upwards, read/write) |
|     x .. 2M-1   | thread  | local stack (growing downwards, read/write) |
|    2M .. x      | [process](https://github.com/bztsrc/osz/tree/master/docs/process.md)  | user program text segment (read only) |
|     x .. 4G-1   | process | shared libraries (read only / read/write) |
|    4G .. 2^56   | process | dynamically allocated bss memory (growing upwards, read/write) |

Normal userspace processes do not have any MMIO, only physical RAM can be mapped in their bss segment.
If two mappings are identical save the TCB and message queue, their threads belong to the same process.

The maximum number of pending events in a queue is a boot time parameter and can be set in `etc/CONFIG` with "nrmqmax". It's given
in pages, so multiply by page size and devide by sizeof(msg_t). Defaults to 1 page, meaning 4096/32 = up to 128 pending events.

Core Memory
-----------

The pages for the core are marked as supervisor only, meaning userspace programs cannot access it.

| Virtual Address | Description |
| --------------- | ----------- |
|   -1G .. -4M-1  | Framebuffer mapped (for kprintf and kpanic) |
|   -4M .. -2M-1  | Destination's message queue. Mapped by msg_send() |
|   -2M .. x      | Core text segment and bss (growing upwards) |
|     x .. 0      | Core stack (growing downwards) |

Process Memory
--------------

Shared among threads, just as user bss.

| Virtual Address | Description |
| --------------- | ----------- |
|   2M .. a-1     | ELF executable's text segment, read-only |
|    a .. b-1     | executable's data segment, read-write |
|    b .. c-1     | libc shared object's text segment, read-only |
|    c ..         | libc's data segment, read-write |
|    ...          | more libraries may follow |

Page Management
---------------

Message buffers are mapped read/write, so libraries can modify it's contents. No page fault occurs accessing it.

On the other hand page faults can occur in the bss section quite often. Mostly because after free or realloc
physical pages are also freed so the virtual address space is fragmented. The good side is no more physical pages
were allocated for higher addresses, so it's RAM efficient. When either a read or write cause a page fault in
thread local bss segment, a new physical page is allocated transparent to the instruction causing the fault.

Shared pages are mapped read/write for the first time, and read only on subsequent times. When a write protection
error in the shared address range occurs, the page will be copied to a new page and the write will affect that
new page. So until a shared page is written, it's real-time updated. After the write it becames a thread local page.

When a physical page is mapped for the second time (first time read-only), bit 9 in PT entries are set in both
thread's page tables.

When a virtual page is freed with bit 9 set, all thread's page tables searched for the same address. Would it be the
last reference and physical page freed as well.

If the system runs out of free physical pages and there's a swap disk configured then the thread with the highest
blkcnt score will be written out to disk, and allocation will continue transparently. Swapped out threads consume
only one page of physical RAM, their TCB. All those TCBs are in tcb_state_hybernated state.
