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
#define IDT_NMI 0x8F020008
#define IDT_INT 0x8E030008
#define IDT_GATE_LO(type,offset) ((uint64_t)((((uint64_t)(offset)>>16)&(uint64_t)0xFFFF)<<48) | (uint64_t)((uint64_t)(type)<<16) | ((uint64_t)(offset) & (uint64_t)0xFFFF))
#define IDT_GATE_HI(offset) ((uint64_t)(offset)>>32)
#define ISR_MAX 128
#define ISR_STACK 64
