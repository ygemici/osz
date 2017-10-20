/*
 * core/x86_64/acpi/platform.h
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
 * @brief Platform specific definitions
 */

#define ISR_CTRL CTRL_APIC
//#define ISR_CTRL CTRL_x2APIC

// PIC constants
#define PIC_MASTER		0x20
#define PIC_MASTER_DATA	0x21
#define PIC_SLAVE		0xA0
#define PIC_SLAVE_DATA	0xA1

// APIC
#define APIC_APICID		0x20
#define APIC_APICVER	0x30
#define APIC_TASKPRI	0x80
#define APIC_EOI		0x0B0
#define APIC_LDR		0x0D0
#define APIC_DFR		0x0E0
#define APIC_SPURIOUS	0x0F0
#define APIC_ISR		0x100
#define APIC_TMR		0x180
#define APIC_IRR		0x200
#define APIC_ESR		0x280
#define APIC_ICRL		0x300
#define APIC_ICRH		0x310
#define APIC_LVT_TMR	0x320
#define APIC_LVT_PERF	0x340
#define APIC_LVT_LINT0	0x350
#define APIC_LVT_LINT1	0x360
#define APIC_LVT_ERR	0x370
#define APIC_TMRINITCNT	0x380
#define APIC_TMRCURRCNT	0x390
#define APIC_TMRDIV		0x3E0
#define APIC_LAST		0x38F
#define APIC_DISABLE	0x10000
#define APIC_SW_ENABLE	0x100
#define APIC_CPUFOCUS	0x200
#define TMR_PERIODIC	0x20000
#define TMR_BASEDIV		(1<<20)

// x2APIC
#define x2APIC_APICID		0x0802
#define x2APIC_APICVER		0x0803
#define x2APIC_TASKPRI		0x0808
#define x2APIC_EOI			0x080B
#define x2APIC_LDR			0x080D
#define x2APIC_SPURIOUS		0x080F
#define x2APIC_ISR			0x0810
#define x2APIC_TMR			0x0818
#define x2APIC_IRR			0x0820
#define x2APIC_ESR			0x0828
#define x2APIC_ICRL			0x0830
#define x2APIC_ICRH			0x0831
#define x2APIC_LVT_TMR		0x0832
#define x2APIC_LVT_PERF		0x0834
#define x2APIC_LVT_LINT0	0x0835
#define x2APIC_LVT_LINT1	0x0836
#define x2APIC_LVT_ERR		0x0837
#define x2APIC_TMRINITCNT	0x0838
#define x2APIC_TMRCURRCNT	0x0839
#define x2APIC_TMRDIV		0x083E
#define x2APIC_SELFIPI		0x083F

// IOAPIC
#define IOAPIC_ID		0
#define IOAPIC_VER		1
#define IOAPIC_AID		2
#define IOAPIC_IRQ0		0x10
#define IOAPIC_IRQ1		0x12
#define IOAPIC_IRQ2		0x14
#define IOAPIC_IRQ3		0x16
#define IOAPIC_IRQ4		0x18
#define IOAPIC_IRQ5		0x1A
#define IOAPIC_IRQ6		0x1C
#define IOAPIC_IRQ7		0x1E
#define IOAPIC_IRQ8		0x20
#define IOAPIC_IRQ9		0x22
#define IOAPIC_IRQ10	0x24
#define IOAPIC_IRQ11	0x26
#define IOAPIC_IRQ12	0x28
#define IOAPIC_IRQ13	0x2A
#define IOAPIC_IRQ14	0x2C
#define IOAPIC_IRQ15	0x2E
#define IOAPIC_PCIA		0x30
#define IOAPIC_PCIB		0x32
#define IOAPIC_PCIC		0x34
#define IOAPIC_PCID		0x36

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
