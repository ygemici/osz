OS/Z Security
=============

The main inter process communication form of OS/Z tasks are message queues. The message queue is mapped read-only, meaning
it can only be modified by the supervisor through `libc` [functions](https://github.com/bztsrc/osz/blob/master/docs/messages.md).

The messages are checked when sent to the queue in [src/core/msg.c](https://github.com/bztsrc/osz/blob/master/src/core/msg.c).
If a message is not valid, it won't be queued. Therefore when a task receives a message it can be sure that the message is valid,
no further security checks are required on the receiver's side.

When a message is not valid, the `libc` error number is set to `EPERM`, meaning permission denied.

Access rights
-------------

Some message types are checked through common criterias. For example only a task in driver priority level allowed to send
a SYS_setirq message to modify the IRQ Routing Table. Likewise the SYS_mknod message is limited to the tasks for priority level
driver and service. Sometimes the message is associated with a specific system service, like only "init" allowed to send
SYS_mountfs message to "FS".

Other messages use more sophisticated way, so called Access Control List check. A good example is SYS_stime message which sets the
system time. Only tasks that has a write Access Control Entry for "stime" are allowed to make that call.

Security checks
---------------

All checks are gathered into one file, in [src/core/security.c](https://github.com/bztsrc/osz/blob/master/src/core/security.c), so
it's easy to verify validity, and apply new security policy.

 * `task_allowed(task, resource, mode)` checks whether the given task has mode (read / write) access for the resource.
 * `msg_allowed(task, dest, messagetype)` checks whether the given task is allowed to send messagetype to dest.

As mentioned earlier, only the `core` can put messages in the queue, and it checks their validity before doing so. Because
message queue is read-only from user space, there's no way a malicious shared library could alter it.

File permissions
----------------

[Unlike POSIX](https://github.com/bztsrc/osz/blob/master/docs/posix.md), OS/Z does not use groups. Instead it uses ACLs
(or list of groups) systemwide. Each Access Control Entry is associated with access rights, like read / write / append etc.
Only a task can perform a specific operation on the file that has at least one ACE that required.

For example: let's say a/b/c is a file with `wheel:w,admin:r`. This means that tasks with `admin:r` access can read the file,
but they cannot modify it unless they also have a `wheel:w` ACE. Usually tasks are given ACEs with full rights, like `wheel:rwxad`.

