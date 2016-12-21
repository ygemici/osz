/*
 * core/x86_64/isr.h
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
 * @brief Interrupt Service Routines header file
 */

// IDT constants
#define IDT_EXC 0xEF010008
#define IDT_NMI 0xEF020008
#define IDT_INT 0xEE030008
#define IDT_GATE_LO(type,offset) ((uint64_t)((((uint64_t)(offset)>>16)&(uint64_t)0xFFFF)<<48) | (uint64_t)((uint64_t)(type)<<16) | ((uint64_t)(offset) & (uint64_t)0xFFFF))
#define IDT_GATE_HI(offset) ((uint64_t)(offset)>>32)
#define ISR_MAX 128
#define ISR_STACK 64

// PIC, PIT constants
#define PIC_MASTER		0x20
#define PIC_MASTER_DATA	0x21
#define PIC_SLAVE		0xA0
#define PIC_SLAVE_DATA	0xA1

// APIC
#define APIC_APICID		20h
#define APIC_APICVER	30h
#define APIC_TASKPRI	80h
#define APIC_EOI		0B0h
#define APIC_LDR		0D0h
#define APIC_DFR		0E0h
#define APIC_SPURIOUS	0F0h
#define APIC_ISR		100h
#define APIC_TMR		180h
#define APIC_IRR		200h
#define APIC_ESR		280h
#define APIC_ICRL		300h
#define APIC_ICRH		310h
#define APIC_LVT_TMR	320h
#define APIC_LVT_PERF	340h
#define APIC_LVT_LINT0	350h
#define APIC_LVT_LINT1	360h
#define APIC_LVT_ERR	370h
#define APIC_TMRINITCNT	380h
#define APIC_TMRCURRCNT	390h
#define APIC_TMRDIV		3E0h
#define APIC_LAST		38Fh
#define APIC_DISABLE	10000h
#define APIC_SW_ENABLE	100h
#define APIC_CPUFOCUS	200h
#define TMR_PERIODIC	20000h
#define TMR_BASEDIV		(1<<20)

// x2APIC
#define x2APIC_APICID		0802h
#define x2APIC_APICVER		0803h
#define x2APIC_TASKPRI		0808h
#define x2APIC_EOI			080Bh
#define x2APIC_LDR			080Dh
#define x2APIC_SPURIOUS		080Fh
#define x2APIC_ISR			0810h
#define x2APIC_TMR			0818h
#define x2APIC_IRR			0820h
#define x2APIC_ESR			0828h
#define x2APIC_ICRL			0830h
#define x2APIC_ICRH			0831h
#define x2APIC_LVT_TMR		0832h
#define x2APIC_LVT_PERF		0834h
#define x2APIC_LVT_LINT0	0835h
#define x2APIC_LVT_LINT1	0836h
#define x2APIC_LVT_ERR		0837h
#define x2APIC_TMRINITCNT	0838h
#define x2APIC_TMRCURRCNT	0839h
#define x2APIC_TMRDIV		083Eh
#define x2APIC_SELFIPI		083Fh

// IOAPIC
#define IOAPIC_ID		0
#define IOAPIC_VER		1
#define IOAPIC_AID		2
#define IOAPIC_IRQ0		10h
#define IOAPIC_IRQ1		12h
#define IOAPIC_IRQ2		14h
#define IOAPIC_IRQ3		16h
#define IOAPIC_IRQ4		18h
#define IOAPIC_IRQ5		1Ah
#define IOAPIC_IRQ6		1Ch
#define IOAPIC_IRQ7		1Eh
#define IOAPIC_IRQ8		20h
#define IOAPIC_IRQ9		22h
#define IOAPIC_IRQ10	24h
#define IOAPIC_IRQ11	26h
#define IOAPIC_IRQ12	28h
#define IOAPIC_IRQ13	2Ah
#define IOAPIC_IRQ14	2Ch
#define IOAPIC_IRQ15	2Eh
#define IOAPIC_PCIA		30h
#define IOAPIC_PCIB		32h
#define IOAPIC_PCIC		34h
#define IOAPIC_PCID		36h

#define IOAPIC_FIXED	(0<<8)
#define IOAPIC_LOWEST	(1<<8)
#define IOAPIC_SMI		(2<<8)
#define IOAPIC_NMI		(4<<8)
#define IOAPIC_INIT		(5<<8)
#define IOAPIC_EXTINT	(7<<8)

#define IOAPIC_PHYSICAL	(0<<11)
#define IOAPIC_LOGICAL	(1<<11)

#define IOAPIC_IDLE		(0<<12)
#define IOAPIC_PENDING	(1<<12)

#define IOAPIC_HIGH		(0<<13)
#define IOAPIC_LOW		(1<<13)

#define IOAPIC_IRROK	(0<<14)
#define IOAPIC_IRREOI	(1<<14)

#define IOAPIC_EDGE		(0<<15)
#define IOAPIC_LEVEL	(1<<15)

#define IOAPIC_UNMASK	(0<<16)
#define IOAPIC_MASK		(1<<16)

#define IOAPIC_DEST		(0<<18)
#define IOAPIC_SELF		(1<<18)
#define IOAPIC_INCL		(2<<18)
#define IOAPIC_EXCL		(3<<18)

#define IOAPIC_IRQMASK	(IOAPIC_EDGE | IOAPIC_HIGH | IOAPIC_FIXED | IOAPIC_PHYSICAL | IOAPIC_UNMASK)
#define IOAPIC_PCIMASK	(IOAPIC_LEVEL | IOAPIC_LOW | IOAPIC_LOWEST | IOAPIC_LOGICAL | IOAPIC_UNMASK)
