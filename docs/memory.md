OS/Z Memory Management
======================

Memory is allocated at several levels:
 - pmm_alloc() allocates physical RAM pages
 - kalloc() allocates core (kernel) bss memory
 - malloc(), realloc(), calloc(), free() allocate user space bss memory
 - smalloc(), srealloc(), scalloc(), sfree() allocate shared memory

Memory Mapping
--------------

All tasks have their own memory mappings. Some pages are different,
some are shared. Device [drivers](https://github.com/bztsrc/osz/tree/master/docs/drivers.md) are a special tasks,
with a slightly different mapping, as MMIO areas are writable in their bss segment. They are also allowed to 
execute in/out instructions, and therefore access IO address space.

User Tasks
----------

| Virtual Address  | Scope   | Description |
| ---------------- | ------- | ----------- |
| -2^56 .. -512M-1 | global  | global shared memory space (user accessible, read/write) |
| -512M .. 0       | global  | core memory (supervisor only) |
|     0 .. 4096    | thread  | Thread Control Block (read-only) |
|  4096 .. x       | thread  | Message Queue (read/write, growing upwards) |
|     x .. 2M-1    | thread  | local stack (read/write, growing downwards) |
|    2M .. x       | [process](https://github.com/bztsrc/osz/tree/master/docs/process.md) | user program text segment (read only) |
|     x .. 4G-4M-1 | process | shared libraries (text read only / data read/write) |
| 4G-4M .. 4G-2M-1 | thread  | list of open files (each 8 bytes, first is a counter) |
| 4G-2M .. 4G-1    | thread  | libc memory allocator internal data |
|    4G .. 2^56    | thread  | dynamically allocated bss memory (growing upwards, read/write) |
|   64G .. 2^56    | thread  | pre-allocated mapped system buffers (screen, initrd) |

Normal userspace tasks do not have any MMIO, only physical RAM can be mapped in their bss segment.
If two mappings are identical save the TCB and message queue, their threads belong to the same process.

The maximum number of pending events in a queue is a boot time parameter and can be set in [etc/CONFIG](https://github.com/bztsrc/osz/tree/master/etc/CONFIG) with "nrmqmax". It's given
in pages, so multiply by page size and devide by sizeof(msg_t). Defaults to 1 page, meaning 4096/64 = up to 64 pending events.

### UI

User interface has one half of a double screen buffer mapped at 64G.

### Display driver

The display driver has the other half of the double screen buffer mapped at 64G, and on next page directory address, the framebuffer.
The screen buffers among UI and the driver tasks swapped by the [vid_swapbuf()](https://github.com/bztsrc/osz/tree/master/src/lib/libc/x86_64/video.S) libc call
(which sends a [SYS_swapbuf](https://github.com/bztsrc/osz/tree/master/src/core/syscall.c) message).
That call is issued by UI task when it finishes the composition on it's screen buffer.

### FS

Filesystem service has the initrd mapped at 64G. It does not use malloc(), instead has a list of open files at 4G, and
per thread specific list at 56G (the table indexed by entries in user task's 4G-4M list).

Core Memory
-----------

The pages for the core are marked as supervisor only, meaning userspace programs cannot access it. They are also globally
mapped to every task's address space.

| Virtual Address | Description |
| --------------- | ----------- |
| -512M .. -2M-1  | Framebuffer mapped (for kprintf and kpanic) |
|   -2M .. x      | [Core text segment](https://github.com/bztsrc/osz/tree/master/src/core/main.c) and bss (growing upwards) |
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

The elf's local data segment comes right after the text segment, so it can take advantage of
rip-relative (position independent) addressing.

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
