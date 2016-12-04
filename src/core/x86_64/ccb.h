/*
 * x86_64/ccb.h
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
 * @brief CPU Control Block. Architecture specific implementation
 * MUST ALIGN WITH 64 BIT TSS
 */

#define OSZ_CCB_MAGIC "CPUB"
#define OSZ_CCB_MAGICH 0x42555043

// index to hd_active
enum {
	PRI_SYS, // priority 0, system, non-interruptible
	PRI_RT,  // priority 1, real time tasks queue
	PRI_DRV, // priority 2, device drivers queue
	PRI_SRV, // priority 3, service queue
	PRI_APPH,// priority 4, application high priority queue
	PRI_APP, // priority 5, application normal priority queue
	PRI_APPL,// priority 6, application low priority queue
	PRI_IDLE // priority 7, idle queue (defragmenter, screensaver etc.)
};

typedef struct {
	uint32_t magic;
	uint64_t rsp0;
	uint64_t rsp1;
	uint64_t rsp2;
	uint16_t realid;	// real APIC ID
	uint16_t id;		// logical APIC ID
	uint32_t minprior;	// firwt task at this priority level
	uint64_t ist1;		// user (exception, syscall) stack
	uint64_t ist2;		// NMI stack
	uint64_t ist3;		// IRQ stack
	uint64_t ist4;		// not used at the moment
	uint64_t mutex;
	uint64_t excerr;	// exception error code
	uint64_t lastxreg;	// pointer to the tcb last used extra registers
	OSZ_tcb *hd_timerq;	// timer queue head (alarm)
	uint32_t dummy;
	uint32_t iomapbase;	// IO permission map base
	OSZ_tcb *hd_blocked;// blocked queue head
	OSZ_tcb *hd_active[8];	// priority queues (active threads)
} __attribute__((packed)) OSZ_ccb;
