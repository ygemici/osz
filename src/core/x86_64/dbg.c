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
#include "../font.h"

extern uint32_t fg;
extern uint32_t bg;
extern int ky;
extern int kx;
extern int fx;
extern int maxx;
extern int maxy;
extern OSZ_ccb ccb;
extern uint64_t *safestack;
extern uint8_t sys_pgfault;

extern uchar *service_sym(virt_t addr);
extern void kprintf_putchar(int c);

uint8_t __attribute__ ((section (".data"))) dbg_enabled;
uint8_t __attribute__ ((section (".data"))) dbg_active;
uint8_t __attribute__ ((section (".data"))) dbg_tab;
uint64_t __attribute__ ((section (".data"))) cr2;
uint64_t __attribute__ ((section (".data"))) cr3;
uint64_t __attribute__ ((section (".data"))) origist;

enum {
    tab_tcb,
    tab_code,
    tab_data,

    tab_last
};

void dbg_tcb()
{
//    OSZ_tcb *tcb = (OSZ_tcb*)0;
}

void dbg_code(uint64_t rip)
{
    OSZ_tcb *tcb = (OSZ_tcb*)0;
    uchar *symstr;
    int i=0;
    uint64_t *rsp=(uint64_t*)tcb->rsp;
    uint64_t m;

    /* registers and stack dump */
    fg=0xCc6c4b; bg=0x200000;
    kprintf("[Registers]");
    kx = maxx-44;
    kprintf("[Stack %8x]\n",tcb->rsp);

    fg=0x9c3c1b;
    kprintf("cs=%4x rip=%8x \n",tcb->cs,tcb->rip);
    kprintf("rflags=%8x excerr=%4x \n",tcb->rflags,tcb->excerr);
    kprintf("cr2=%8x cr3=%8x \n\n",cr2,cr3);
    kprintf("rax=%8x rbx=%8x \n",*((uint64_t*)tcb->gpr),*((uint64_t*)tcb->gpr+8));
    kprintf("rcx=%8x rdx=%8x \n",*((uint64_t*)tcb->gpr+16),*((uint64_t*)tcb->gpr+24));
    kprintf("rsi=%8x rdi=%8x \n",*((uint64_t*)tcb->gpr+32),*((uint64_t*)tcb->gpr+40));
    kprintf("rbp=%8x rsp=%8x \n",*((uint64_t*)tcb->gpr+112),tcb->rsp);
    kprintf(" r8=%8x  r9=%8x \n",*((uint64_t*)tcb->gpr+48),*((uint64_t*)tcb->gpr+56));
    kprintf("r10=%8x r11=%8x \n",*((uint64_t*)tcb->gpr+64),*((uint64_t*)tcb->gpr+72));
    kprintf("r12=%8x r13=%8x \n",*((uint64_t*)tcb->gpr+80),*((uint64_t*)tcb->gpr+88));
    kprintf("r14=%8x r15=%8x \n",*((uint64_t*)tcb->gpr+96),*((uint64_t*)tcb->gpr+104));

    /* print out current task and last message */
    kx=fx=maxx-40; ky++;
    fg=0xCc6c4b;
    kprintf("[Last message]\n");
    fg=0x9c3c1b;
    m = *((uint64_t*)MQ_ADDRESS) - 1;
    if(m==0)
        m = *((uint64_t*)(MQ_ADDRESS+16)) - 1;
    m = ((m * sizeof(msg_t))+(uint64_t)MQ_ADDRESS);
    kprintf(" @%x\n%8x %8x\n", m,
        *((uint64_t*)m), *((uint64_t*)(m+8)));
    kprintf("%8x %8x\n%8x %8x\n",
        *((uint64_t*)(m+16)), *((uint64_t*)(m+24)),
        *((uint64_t*)(m+32)), *((uint64_t*)(m+40)));
    kprintf("%8x %8x\n",
        *((uint64_t*)(m+48)), *((uint64_t*)(m+56)));

    /* stack */
    ky=3; kx=fx=maxx-44;
    sys_pgfault = false;
    while(i++<11 && !sys_pgfault && ((uint64_t)rsp<(uint64_t)TEXT_ADDRESS||(uint64_t)rsp>(uint64_t)&__bss_start)){
        kprintf("rsp+%1x rbp-%1x: %8x \n",
            (uint32_t)(uint64_t)rsp - (uint64_t)tcb->rsp,
            (uint32_t)((uint64_t)(tcb->gpr+112)-(uint64_t)rsp),
            *rsp
        );
        rsp++;
    }
    kprintf(sys_pgfault?"  * page fault *\n":"  * end *\n");

    /* back trace */
    ky=16; kx=fx=0;
    fg=0xCc6c4b;
    kprintf("[Back trace]\n");
    fg=0x9c3c1b;
    rsp=(uint64_t*)tcb->rsp;
    i=0;
    kprintf("%8x: %s \n",
        tcb->rip, service_sym(tcb->rip)
    );
    sys_pgfault = false;
    while(i++<8 && !sys_pgfault && rsp!=0 && ((uint64_t)rsp<(uint64_t)TEXT_ADDRESS||(uint64_t)rsp>(uint64_t)&__bss_start)){
        if((((rsp[1]==0x23||rsp[1]==0x08) &&
            (rsp[4]==0x1b||rsp[4]==0x18)) || tcb->rsp==__PAGESIZE-40) && !sys_pgfault) {
            kprintf("%8x: %s \n  * interrupt * \n",tcb->rip, service_sym(tcb->rip));
            rsp=(uint64_t*)rsp[3];
            continue;
        }
        if(sys_pgfault)
            break;
        if((*rsp>TEXT_ADDRESS && *rsp<BSS_ADDRESS) ||
           (*rsp>((uint64_t)CORE_ADDRESS) &&
           (uint64_t)*rsp<(uint64_t)&__bss_start)) {
            if(sys_pgfault)
                break;
            symstr = service_sym(*rsp);
            if(sys_pgfault)
                break;
            if((virt_t)symstr>(virt_t)TEXT_ADDRESS &&
                (virt_t)symstr<(virt_t)BSS_ADDRESS && kstrcmp(symstr,"_edata"))
                kprintf("%8x: %s \n", *rsp, symstr);
        }
        rsp++;
    }
    kprintf(sys_pgfault?"  * page fault *\n":"  * end *\n");
    sys_pgfault = false;
    /* TODO: disasm */
}

void dbg_data()
{
}

void dbg_init()
{
    dbg_enabled = 1;
}

void dbg_enable(uint64_t rip, char *reason)
{
    OSZ_tcb *tcb = (OSZ_tcb*)0;
    uint64_t scancode, offs, line, x, y;
    OSZ_font *font = (OSZ_font*)&_binary_font_start;
    char *tabs[] = { "TCB", "Code", "Memory" };
    char cmd[64];
    int cmdidx=0,cmdlast=0;

    dbg_active = true;
    __asm__ __volatile__ ( "movq %%cr2, %%rax; movq %%rax, %0" : "=r"(cr2) : : "%rax");
    __asm__ __volatile__ ( "movq %%cr3, %%rax; movq %%rax, %0" : "=r"(cr3) : : "%rax");
    ccb.ist1 = (uint64_t)safestack + (uint64_t)__PAGESIZE-1024;

    if(reason!=NULL&&*reason!=0) {
        kprintf_reset();
        fg = 0xFFDD33;
        bg = 0;
        kprintf("OS/Z core debug: %s  \n", reason);
    }
    offs = font->height*bootboot.fb_scanline;
    for(y=0;y<font->height;y++){
        line=offs;
        for(x=0;x<bootboot.fb_width;x++){
            *((uint32_t*)(FBUF_ADDRESS + line))=(uint32_t)0;
            line+=4;
        }
        offs+=bootboot.fb_scanline;
    }

    dbg_tab = tab_code;
    /* redraw and get command */
    while(1) {
        fx=kx=0; ky=1;
        for(x=0;x<tab_last;x++) {
            kx+=2;
            bg= dbg_tab==x ? 0x200000 : 0;
            fg= dbg_tab==x ? 0xCCAA11 : 0x604010;
            kprintf_putchar(' ');
            kx++;
            kprintf(tabs[x]);
            kprintf_putchar(' ');
            kx++;
        }
        fg = 0x404040; bg=0; kx = maxx-8-kstrlen((char*)tcb->name);
        kprintf("(%x) %s", tcb->mypid, tcb->name);
        // clear tab
        offs = font->height*bootboot.fb_scanline*2;
        for(y=font->height;y<(maxy-2)*font->height;y++){
            line=offs;
            for(x=0;x<bootboot.fb_width;x++){
                *((uint32_t*)(FBUF_ADDRESS + line))=(uint32_t)0x200000;
                line+=4;
            }
            offs+=bootboot.fb_scanline;
        }
        // draw tab
        fx=kx=0; ky=2;
        __asm__ __volatile__ ("pushq %%rdi":::"%rdi");
        if(dbg_tab==tab_tcb) dbg_tcb(); else
        if(dbg_tab==tab_code) dbg_code(rip); else
        if(dbg_tab==tab_data) dbg_data();
        __asm__ __volatile__ ("popq %%rdi":::"%rdi");
        offs = (maxy-2)*font->height*bootboot.fb_scanline;
        // debugger command line
        for(y=(maxy-2)*font->height;y<bootboot.fb_height;y++){
            line=offs;
            for(x=0;x<bootboot.fb_width;x++){
                *((uint32_t*)(FBUF_ADDRESS + line))=(uint32_t)0x0;
                line+=4;
            }
            offs+=bootboot.fb_scanline;
        }
        fx=kx=0; ky=maxy; ky-=2; fg=0x808080; bg=0;
        cmd[cmdidx]=0;
        kprintf("dbg> %s",cmd);
        fg=0; bg=0x404040;
        kx=5+cmdidx;
        kprintf_putchar(cmd[cmdidx]==0?' ':cmd[cmdidx]);
        scancode = kwaitkey();
        // ESC
        if(scancode == 1) {
            dbg_active = false;
            ccb.ist1 = __PAGESIZE;
            return;
        }
        // Tab
        if(scancode == 15) {
            dbg_tab++;
            if(dbg_tab>=tab_last)
                dbg_tab=0;
            continue;
        }
    }
    breakpoint;
    asm("cli;hlt");
}

#endif
