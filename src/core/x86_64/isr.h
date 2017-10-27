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

//controllers
#define CTRL_PIC    0
#define CTRL_APIC   1
#define CTRL_x2APIC 2

//timers
#define TMR_HPET    1
#define TMR_PIT     2
#define TMR_RTC     3

// IDT constants
// 1ddd0tttt0000iiiissssssssssssssss
// d: dpl, t: F trap, E int, i: ist, s: code selector
#define IDT_EXC 0x8F010008
#define IDT_NMI 0x8F020008
#define IDT_INT 0x8E010008
#define IDT_DBG 0xEF030008
#define IDT_GATE_LO(type,offset) ((uint64_t)((((uint64_t)(offset)>>16)&(uint64_t)0xFFFF)<<48) | (uint64_t)((uint64_t)(type)<<16) | ((uint64_t)(offset) & (uint64_t)0xFFFF))
#define IDT_GATE_HI(offset) ((uint64_t)(offset)>>32)

/* if you change  one of this, you'll have to run isrs.sh */
#define ISR_NUMIRQ 32       //maximum number of IRQ lines
#define ISR_EXCMAX 128      //maximum code size of exception ISRs
#define ISR_IRQMAX 128      //maximum code size of IRQ ISRs
#define ISR_STACK 64        //minimum size of stack for ISRs
