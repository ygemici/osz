/*
 * core/x86_64/dbg.c
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
 * @brief PCI functions and enumeration
 */

#if DEBUG

#include "platform.h"
#include "../env.h"

extern uint32_t fg;
extern uint32_t bg;
extern int ky;
extern int kx;
extern int fx;

extern uchar *service_sym(virt_t addr);

uint8_t __attribute__ ((section (".data"))) dbg_enabled;

void dbg_init()
{
    dbg_enabled = 1;
}

void dbg_enable(uint64_t rip, char *reason)
{
    OSZ_tcb *tcb = (OSZ_tcb*)0;
    uint64_t cr2, cr3;
    int i=0;
    uint64_t *rsp=(uint64_t*)tcb->rsp;
    __asm__ __volatile__ ( "movq %%cr2, %%rax; movq %%rax, %0" : "=r"(cr2) : : "%rax"); 
    __asm__ __volatile__ ( "movq %%cr3, %%rax; movq %%rax, %0" : "=r"(cr3) : : "%rax"); 
    if(reason!=NULL&&*reason!=0) {
        kprintf_reset();
        fg = 0xFFDD33;
        bg = 0;
        kprintf("OS/Z core debug: %s  \n", reason);
    }
    fx=kx=0; ky=2;
    fg=0xCc6c4b; bg=0x200000;
    /* registers */
    kprintf("[Registers]");
    kx = 44;
    kprintf("[Stack]\n");

    fg=0x9c3c1b; bg=0;
    kprintf("cs=%4x rip=%8x \n",tcb->cs,tcb->rip);
    kprintf("rflags=%8x excerr=%d \n",tcb->rflags,tcb->excerr);
    kprintf("cr2=%8x cr3=%8x \n\n",cr2,cr3);
    kprintf("rax=%8x rbx=%8x \n",tcb->gpr,tcb->gpr+8);
    kprintf("rcx=%8x rdx=%8x \n",tcb->gpr+16,tcb->gpr+24);
    kprintf("rsi=%8x rdi=%8x \n",tcb->gpr+32,tcb->gpr+40);
    kprintf("rbp=%8x rsp=%8x \n",tcb->gpr+112,tcb->rsp);
    kprintf(" r8=%8x  r9=%8x \n",tcb->gpr+48,tcb->gpr+56);
    kprintf("r10=%8x r11=%8x \n",tcb->gpr+64,tcb->gpr+72);
    kprintf("r12=%8x r13=%8x \n",tcb->gpr+80,tcb->gpr+88);
    kprintf("r14=%8x r15=%8x \n",tcb->gpr+96,tcb->gpr+104);
    /* stack */
    ky=3; kx=fx=44;
    if((uint64_t)rsp<(uint64_t)TEXT_ADDRESS || (uint64_t)rsp>(uint64_t)((uint64_t)&environment+(uint64_t)__PAGESIZE)) {
        while(i++<12){
            kprintf("rsp+%1x rbp-%1x: %8x \n",
                (uint32_t)(uint64_t)rsp - (uint64_t)tcb->rsp,
                (uint32_t)((uint64_t)(tcb->gpr+112)-(uint64_t)rsp),
                *rsp
            );
            rsp++;
        }
        /* back trace */
        ky+=2; kx=fx=0;
        fg=0xCc6c4b; bg=0x200000;
        kprintf("[Back trace]\n");
        fg=0x9c3c1b; bg=0;
        rsp=(uint64_t*)tcb->rsp;
        i=0;
        kprintf("%8x: %s \n",
            tcb->rip, service_sym(tcb->rip)
        );
        while(i++<64){
            if(((rsp[1]==0x23||rsp[1]==0x08) &&
                (rsp[4]==0x1b||rsp[4]==0x18)) || tcb->rsp==__PAGESIZE-40) {
                kprintf("%8x: %s \n  * interrupt * \n",tcb->rip, service_sym(tcb->rip));
                rsp=(uint64_t*)rsp[3];
                continue;
            }
            if((*rsp>TEXT_ADDRESS && *rsp<BSS_ADDRESS) ||
               (*rsp>((uint64_t)&environment+(uint64_t)__PAGESIZE) &&
               (uint64_t)*rsp<(uint64_t)&__bss_start)) {
                kprintf("%8x: %s \n",
                    *rsp, service_sym(*rsp)
                );
            }
            rsp++;
        }
    } else {
        kprintf("  * not available *\n");
    }
    /* TODO: debugger, disasm, step through */
    breakpoint;
    asm("cli;hlt");
}

#endif
