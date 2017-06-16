/*
 * sys/mman.h
 *
 * Copyright 2016 CC-by-nc-sa bztsrc@github
 * https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * You are free to:
 *
 * - Share — copy and redistribute the material in any medium or format
 * - Adapt — remix, transform, and build upon the material
 *     The licensor cannot revoke these freedoms as long as you follow
 *     the license terms.
 *
 * Under the following terms:
 *
 * - Attribution — You must give appropriate credit, provide a link to
 *     the license, and indicate if changes were made. You may do so in
 *     any reasonable manner, but not in any way that suggests the
 *     licensor endorses you or your use.
 * - NonCommercial — You may not use the material for commercial purposes.
 * - ShareAlike — If you remix, transform, or build upon the material,
 *     you must distribute your contributions under the same license as
 *     the original.
 *
 * @brief OS/Z Memory Manager services
 */

#ifndef _SYS_MMAN_H
#define _SYS_MMAN_H 1

#define PROT_NONE   0x0     /* Page can not be accessed.  */
#define PROT_READ   0x1     /* Page can be read.  */
#define PROT_WRITE  0x2     /* Page can be written.  */
#define PROT_EXEC   0x0     /* Page can be executed. Cannot be used in OS/Z */
#define PROT_GROWSDOWN  0x01000000	/* Extend change to start of growsdown vma (mprotect only).  */
#define PROT_GROWSUP    0x02000000	/* Extend change to start of growsup vma (mprotect only).  */

/* Sharing type flags (must choose one and only one of these). */
#define MAP_SHARED  0x01        /* Share changes.  */
#define MAP_PRIVATE 0x02        /* Changes are private.  */

/* Other flags.  */
#define MAP_FIXED       0x10        /* Interpret addr exactly.  */
#define MAP_FILE        0
#define MAP_ANONYMOUS   0x20        /* Don't use a file.  */
#define MAP_ANON        MAP_ANONYMOUS
#define MAP_LOCKED      0x100       /* Lock the mapping.  */
#define MAP_NONBLOCK    0x200       /* Do not block on IO.  */

/* Flags to `msync'.  */
#define MS_ASYNC        1       /* Sync memory asynchronously.  */
#define MS_SYNC         4       /* Synchronous memory sync.  */
#define MS_INVALIDATE   2       /* Invalidate the caches.  */

/* Return value of `mmap' in case of an error.  */
#define MAP_FAILED	((void *) -1)

/*** libc implementation prototypes */
#ifndef _AS
/* Map addresses starting near ADDR and extending for LEN bytes.  from
   OFFSET into the file FD describes according to PROT and FLAGS.  If ADDR
   is nonzero, it is the desired mapping address.  If the MAP_FIXED bit is
   set in FLAGS, the mapping will be at ADDR exactly (which must be
   page-aligned); otherwise the system chooses a convenient nearby address.
   The return value is the actual mapping address chosen or MAP_FAILED
   for errors (in which case `errno' is set).  A successful `mmap' call
   deallocates any previous mapping for the affected region.  */
extern void *mmap (void *addr, size_t len, int prot, int flags, int fd, off_t offset);

/* Deallocate any mapping for the region starting at ADDR and extending LEN
   bytes.  Returns 0 if successful, -1 for errors (and sets errno).  */
extern int munmap (void *addr, size_t len);

/* Change the memory protection of the region starting at ADDR and
   extending LEN bytes to PROT.  Returns 0 if successful, -1 for errors
   (and sets errno).  */
extern int mprotect (void *addr, size_t len, int prot);

/* Synchronize the region starting at ADDR and extending LEN bytes with the
   file it maps.  Filesystem operations on a file being mapped are
   unpredictable before this is done.  Flags are from the MS_* set.  */
extern int msync (void *addr, size_t len, int flags);

/* Guarantee all whole pages mapped by the range [ADDR,ADDR+LEN) to
   be memory resident.  */
extern int mlock (const void *addr, size_t len);

/* Unlock whole pages previously mapped by the range [ADDR,ADDR+LEN).  */
extern int munlock (const void *addr, size_t len);

/* Cause all currently mapped pages of the process to be memory resident
   until unlocked by a call to the `munlockall', until the process exits,
   or until the process calls `execve'.  */
extern int mlockall (int flags);

/* All currently mapped pages of the process' address space become
   unlocked.  */
extern int munlockall (void);

#endif

#endif /* sys/mman.h */
