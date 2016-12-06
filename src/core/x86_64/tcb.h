/*
 * x86_64/tcb.h
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
 * @brief Thread Control Block, architecture specific part
 */

#include "../tcb.h"

#define OSZ_tcb_flag_needsxsave		4
#define OSZ_tcb_flag_needxmmsave	8

typedef struct {
	uint32_t magic;
	uint8_t state;		// thread state
	uint8_t priority;	// thread priority
	uint16_t cpu;		// APIC ID of cpu on which this thread runs
	uint32_t flags;		// thread flags
	uint32_t evtq_cnt;	// event counter (number of items in event queue)
	uint64_t corersp;	// core stack pointer
	uint64_t userrip;	// user instruction pointer on syscall
	uint64_t self;		// self pointer (physical address)
	uint64_t memroot;	// memory mapping root
	uint64_t userflags;	// user space cpu flags
	uint64_t usererrno;	// thread-safe libc errno
	uint64_t linkmem;	// head of linked memory chain
	uint64_t allocmem;	// head of allocated memory chain
	pid_t next;			// next thread in this priority level
	pid_t prev;			// previous thread in this priority level
	uint64_t billcnt;	// ticks thread spent in userspace
	uint64_t syscnt;	// ticks thread spent in kernelspace (core)
	uint64_t evtq_ptr;	// event queue
	uint64_t evtq_endptr;
	uint64_t evtq_nextserial;
	uint64_t evtq_lastserial;
	pid_t recvfrom;
	pid_t sendto;
	pid_t blksend;		// head of blocked threads chain (sending to this thread)
	uint64_t gpr[128];	// general purpose registers save area
	uint8_t fx[512];	// floating point registers save area (16 aligned)
	uint64_t acl[128];	// access control list
} __attribute__((packed)) OSZ_tcb;
