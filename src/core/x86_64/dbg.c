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

#include <lastbuild.h>
#include "../font.h"

extern uint32_t fg;
extern uint32_t bg;
extern int ky;
extern int kx;
extern int fx;
extern int maxx;
extern int maxy;
extern int scry;
extern OSZ_ccb ccb;
extern OSZ_pmm pmm;
extern uint64_t *safestack;
extern uint8_t sys_pgfault;
extern uint64_t lastsym;

extern uchar *service_sym(virt_t addr);
extern void kprintf_putchar(int c);
extern unsigned char *env_hex(unsigned char *s, uint64_t *v, uint64_t min, uint64_t max);
extern virt_t disasm(virt_t rip, char *str);

uint8_t __attribute__ ((section (".data"))) dbg_enabled;
uint8_t __attribute__ ((section (".data"))) dbg_active;
uint8_t __attribute__ ((section (".data"))) dbg_tab;
uint8_t __attribute__ ((section (".data"))) dbg_inst;
uint64_t __attribute__ ((section (".data"))) cr2;
uint64_t __attribute__ ((section (".data"))) cr3;
char __attribute__ ((section (".data"))) dbg_instruction[128];
virt_t __attribute__ ((section (".data"))) dbg_comment;
virt_t __attribute__ ((section (".data"))) dbg_next;
virt_t __attribute__ ((section (".data"))) dbg_start;

enum {
    tab_code,
    tab_tcb,
    tab_msg,
    tab_queues,
    tab_ram,

    tab_last
};

char __attribute__ ((section (".data"))) *prio[] = {
    "SYSTEM   ",
    "RealTime ",
    "Driver   ",
    "Server   ",
    "Important",
    "Normal   ",
    "Low prio ",
    "IDLE ONLY"
};

void dbg_tcb()
{
    OSZ_tcb *tcb = (OSZ_tcb*)0;
    char *states[] = { "hybernated", "blocked", "running" };

    fg=0xCc6c4b;
    kprintf("[Thread State]\n");
    fg=0x9c3c1b;
    kprintf("pid: %x, name: %s\nRunning on cpu core: %x, ",tcb->mypid, tcb->name, tcb->cpu);
    kprintf("priority: %s (%d), state: %s (%1x) ",
        prio[tcb->priority], tcb->priority,
        states[ TCB_STATE(tcb->state)], TCB_STATE(tcb->state)
    );
    if(tcb->state & tcb_flag_alarm)
        kprintf("alarm ");
    if(tcb->state & tcb_flag_io)
        kprintf("iowait ");
    if(tcb->state & tcb_flag_strace)
        kprintf("strace ");
    if(TCB_STATE(tcb->state)==tcb_state_hybernated) {
        fg=0xCc6c4b;
        kprintf("\n[Hybernation info]\n");
        fg=0x9c3c1b;
        kprintf("swapped to: %x\n", tcb->swapminor);
    } else {
        kprintf("\nerrno: %d, exception error code: %d\n", tcb->errno, tcb->excerr);
        fg=0xCc6c4b;
        kprintf("\n[Memory]\n");
        fg=0x9c3c1b;
        kprintf("memory root: %x, allocated pages: %d, linked pages: %d\n", tcb->memroot, tcb->allocmem, tcb->linkmem);
        fg=0xCc6c4b;
        kprintf("\n[Scheduler]\n");
        fg=0x9c3c1b;
        kprintf("user ticks: %d, system ticks: %d, blocked ticks: %d\n", tcb->billcnt, tcb->syscnt, tcb->blkcnt);
        kprintf("next task in queue: %4x, previous task in queue: %4x\n", tcb->next, tcb->prev);
        if(TCB_STATE(tcb->state)==tcb_state_blocked) {
            kprintf("next task in alarm queue: %4x, alarm at: %d.%d\nblocked since: %d", tcb->alarm, tcb->alarmsec, tcb->alarmns, tcb->blktime);
        }
        kprintf("\n");
    }
}

void dbg_code(uint64_t rip)
{
    OSZ_tcb *tcb = (OSZ_tcb*)0;
    uchar *symstr;
    virt_t ptr, dmp, lastsymsave;
    int i=0;
    uint64_t j, *rsp=(uint64_t*)tcb->rsp;

    /* registers and stack dump */
    fx = 2;
    fg=0xCc6c4b;
    kprintf("[Registers]");
    kx = maxx-43;
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

    /* stack */
    ky=4; kx=fx=maxx-43;
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
    ky=17; kx=fx=2;
    fg=0xCc6c4b;
    kprintf("[Back trace]\n");
    fg=0x9c3c1b;
    rsp=(uint64_t*)tcb->rsp;
    i=0;
    kprintf("%8x: %s \n",
        tcb->rip, service_sym(tcb->rip)
    );
    sys_pgfault = false;
    while(i++<4 && !sys_pgfault && rsp!=0 && ((uint64_t)rsp<(uint64_t)TEXT_ADDRESS||(uint64_t)rsp>(uint64_t)&__bss_start)){
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

    /* disassemble block */
    kx=fx=2; ky++;
    fg=0xCc6c4b;
    sys_pgfault = false;
    symstr = service_sym(rip);
    kprintf("[Code %x: ", rip);
    if(sys_pgfault) {
        kprintf("unknown]\n");
        sys_pgfault = false;
        return;
    } else
        kprintf("%s +%2x]\n", symstr, rip-lastsym);
    fg=0x9c3c1b;
    /* disassemble instructions */
    fg=0x9c3c1b;
    /* find a start position */
    j=dbg_start? dbg_start : rip-16;
    dbg_start = 0;
    if(j < lastsym)
        j = lastsym;
    i = 0;
    while(!i && j<=rip) {
        ptr = j;
        sys_pgfault = false;
        while(ky < maxy-2 && !sys_pgfault) {
            if(ptr == rip) {
                i = 1;
                ptr = j;
                break;
            }
            ptr = disasm((virt_t)ptr, NULL);
        }
        j++;
    }
    dbg_next = disasm(ptr, NULL);
    while(ky < maxy-2) {
        fg= ptr==rip?0xFFDD33:0x9c3c1b;
        kprintf("%2x: ",(uint64_t)ptr-lastsym);
        dbg_comment = 0;
        dmp = ptr;
        lastsymsave = lastsym;
        sys_pgfault = false;
        ptr = disasm((virt_t)ptr, dbg_instruction);
        if(sys_pgfault) {
            kprintf(" * page fault *");
            return;
        }
        if(dbg_inst) {
            kprintf("%s", dbg_instruction);
        } else {
            for(;dmp<ptr;dmp++)
                kprintf("%1x ",*((uchar*)dmp));
        }
        if(dbg_comment) {
            fg=0x4c1c0b;
            kx=maxx/2;
            sys_pgfault = false;
            symstr = service_sym(dbg_comment);
            if(!sys_pgfault && symstr!=NULL && symstr[0]!='(')
                kprintf("%s +%x", symstr, dbg_comment-lastsym);
        }
        lastsym = lastsymsave;
        kx=fx; ky++;
    }
}

void dbg_msg()
{
    uint64_t m, i;

    /* print out last message */
    fg=0xCc6c4b;
    kprintf("[Queue Header]\n");
    fg=0x9c3c1b;
    kprintf("msg in: %3d, msg out: %3d, msg max: %3d, recv from: %x\n",
        *((uint64_t*)(MQ_ADDRESS)), *((uint64_t*)(MQ_ADDRESS+8)),
        *((uint64_t*)(MQ_ADDRESS+16)),*((uint64_t*)(MQ_ADDRESS+24)));
    kprintf("buf in: %3d, buf out: %3d, buf max: %3d, buf min: %3d\n",
        *((uint64_t*)(MQ_ADDRESS+32)), *((uint64_t*)(MQ_ADDRESS+40)),
        *((uint64_t*)(MQ_ADDRESS+48)),*((uint64_t*)(MQ_ADDRESS+56)));
    fg=0xCc6c4b;
    kprintf("\n[Messages]");
    i = *((uint64_t*)MQ_ADDRESS) - 1;
    if(i==0)
        i = *((uint64_t*)(MQ_ADDRESS+16)) - 1;
    do {
        fg = i==*((uint64_t*)MQ_ADDRESS)? 0xFFDD33 : 0x9c3c1b;
        m = ((i * sizeof(msg_t))+(uint64_t)MQ_ADDRESS);
        kprintf("\n @%2x %8x %8x ", m,
            *((uint64_t*)m), *((uint64_t*)(m+8)));
        kprintf("%8x %8x\n       %8x %8x ",
            *((uint64_t*)(m+16)), *((uint64_t*)(m+24)),
            *((uint64_t*)(m+32)), *((uint64_t*)(m+40)));
        kprintf("%8x %8x\n",
            *((uint64_t*)(m+48)), *((uint64_t*)(m+56)));
        i++;
        if(i>=*((uint64_t*)(MQ_ADDRESS+16))-1)
            i=1;
    } while (i!=*((uint64_t*)(MQ_ADDRESS+8)) && ky<maxy-3);
}

void dbg_queues()
{
    OSZ_tcb *tcbq = (OSZ_tcb*)&tmpmap;
    int i, j;
    pid_t n;

    fg=0xCc6c4b;
    kprintf("[CPU Control Block]\n");
    fg=0x9c3c1b;
    kprintf("Cpu real id: %d, logical id: %d, ",ccb.realid, ccb.id, ccb.ist1, ccb.ist2);
    kprintf("last xreg: %4x, mutex: %4x%4x%4x\n", ccb.lastxreg, ccb.mutex[0], ccb.mutex[1], ccb.mutex[2]);
    fg=0xCc6c4b;
    kprintf("\n[Blocked Task Queues]\n");
    fg=0x9c3c1b;
    j=0;
    if(ccb.hd_blocked) {
        n = ccb.hd_blocked;
        do {
            j++;
            kmap((virt_t)&tmpmap, n * __PAGESIZE, PG_CORE_NOCACHE);
            n = tcbq->next;
        } while(n!=0);
    }
    kprintf("timerq head: %4x (awake at %d.%d)\niowait head: %4x, %d task(s)\n",
        ccb.hd_timerq, ((OSZ_tcb*)&tmpalarm)->alarmsec, ((OSZ_tcb*)&tmpalarm)->alarmns, ccb.hd_blocked, j);
    fg=0xCc6c4b;
    kprintf("\n[Active Task Queues]\n No Queue     Head      Current   #Tasks\n");
    fg=0x9c3c1b;
    for(i=0;i<8;i++) {
        j=0;
        if(ccb.hd_active[i]) {
            n = ccb.hd_active[i];
            do {
                j++;
                kmap((virt_t)&tmpmap, n * __PAGESIZE, PG_CORE_NOCACHE);
                n = tcbq->next;
            } while(n!=0);
        }
        kprintf(" %d. %s %4x, %4x, %6d\n", i, prio[i], ccb.hd_active[i], ccb.cr_active[i],j);
    }
}

void dbg_ram()
{
    OSZ_pmm_entry *fmem = pmm.entries;
    int i = pmm.size;

    fg=0xCc6c4b;
    kprintf("[Physical Memory Manager]\n");
    fg=0x9c3c1b;
    kprintf("Core bss: %8x - %8x\nNumber of free memory fragments: %d\n\n", pmm.bss, pmm.bss_end,pmm.size);
    kprintf("Total: %9d pages, allocated: %9d pages, free: %9d pages\n",
        pmm.totalpages, pmm.totalpages-pmm.freepages, pmm.freepages
    );
    bg = 0x400000;
    for(i=0;i<(pmm.totalpages-pmm.freepages)*(maxx-12)/(pmm.totalpages+1);i++) {
        kprintf_putchar(' '); kx++;
    }
    bg = 0x100000;
    for(;i<(maxx-12);i++) {
        kprintf_putchar(' '); kx++;
    }
    bg = 0x200000;
    kprintf(" %d.%d%%\n",
        (pmm.totalpages-pmm.freepages)*100/(pmm.totalpages+1), ((pmm.totalpages-pmm.freepages)*1000/(pmm.totalpages+1))%10
    );
    fg=0xCc6c4b;
    kprintf("\n[Free Memory Fragments]\nAddress           Num pages\n");
    fg=0x9c3c1b;
    for(i=pmm.size;i>0;i--) {
        kprintf("%8x, %9d\n",fmem->base, fmem->size);
        fmem++;
    }
}

void dbg_switchprev()
{
}

void dbg_switchnext()
{
}

void dbg_init()
{
    dbg_enabled = 1;
}

void dbg_enable(uint64_t rip, char *reason)
{
    OSZ_tcb *tcb = (OSZ_tcb*)0;
    uint64_t scancode = 0, offs, line, x, y;
    OSZ_font *font = (OSZ_font*)&_binary_font_start;
    char *tabs[] = { "Code", "TCB", "Messages", "CCB", "RAM" };
    char cmd[64], c;
    int cmdidx=0,cmdlast=0;

    scry = -1;
    dbg_active = true;
    __asm__ __volatile__ ( "movq %%cr2, %%rax; movq %%rax, %0" : "=r"(cr2) : : "%rax");
    __asm__ __volatile__ ( "movq %%cr3, %%rax; movq %%rax, %0" : "=r"(cr3) : : "%rax");
    ccb.ist1 = (uint64_t)safestack + (uint64_t)__PAGESIZE-1024;

    if(rip!=tcb->rip)
        rip=tcb->rip;

    dbg_next = dbg_start = 0;

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
    dbg_inst = 1;
    /* redraw and get command */
    while(1) {
redraw:
        fx=kx=0; ky=1;
        for(x=0;x<tab_last;x++) {
            kx+=2;
            bg= dbg_tab==x ? 0x200000 : 0;
            fg= dbg_tab==x ? 0xCc6c4b : 0x400000;
            kprintf_putchar(' ');
            kx++;
            kprintf(tabs[x]);
            kprintf_putchar(' ');
            kx++;
        }
        fg = 0x404040; bg=0; kx = maxx-8-kstrlen((char*)tcb->name);
        kprintf("(%x) %s", tcb->mypid, tcb->name);
        // clear tab
        bg=0x200000;
        offs = font->height*bootboot.fb_scanline*2;
        for(y=font->height;y<(maxy-1)*font->height;y++){
            line=offs;
            for(x=0;x<bootboot.fb_width;x++){
                *((uint32_t*)(FBUF_ADDRESS + line))=(uint32_t)bg;
                line+=4;
            }
            offs+=bootboot.fb_scanline;
        }
        // draw tab
        fx=kx=2; ky=3;
        __asm__ __volatile__ ("pushq %%rdi":::"%rdi");
        if(dbg_tab==tab_code) dbg_code(rip); else
        if(dbg_tab==tab_tcb) dbg_tcb(); else
        if(dbg_tab==tab_msg) dbg_msg(); else
        if(dbg_tab==tab_queues) dbg_queues(); else
        if(dbg_tab==tab_ram) dbg_ram();
        __asm__ __volatile__ ("popq %%rdi":::"%rdi");
        offs = (maxy-1)*font->height*bootboot.fb_scanline;
        // debugger command line
        for(y=(maxy-1)*font->height;y<bootboot.fb_height;y++){
            line=offs;
            for(x=0;x<bootboot.fb_width;x++){
                *((uint32_t*)(FBUF_ADDRESS + line))=(uint32_t)0x0;
                line+=4;
            }
            offs+=bootboot.fb_scanline;
        }
getcmd:
        fx=kx=0; ky=maxy-1; fg=0x808080; bg=0;
        cmd[cmdlast]=0;
        kprintf("dbg> %s  ",cmd);
        fg=0; bg=0x404040;
        kx=5+cmdidx;
        x = cmdidx==cmdlast||cmd[cmdidx]==0?' ':cmd[cmdidx];
        kprintf_putchar(x);
        if(scancode) {
            kx=maxx-4; fg=0x101010; bg=0;
            kprintf("%d",scancode);
        }
        scancode = kwaitkey();
        switch(scancode){
            // ESC
            case 1: {
                dbg_active = false;
                ccb.ist1 = __PAGESIZE;
                return;
            }
            // Tab
            case 15: {
                dbg_tab++;
                if(dbg_tab>=tab_last)
                    dbg_tab=0;
                continue;
            }
            // Left
            case 331: {
                if(cmdlast==0) {
                    dbg_switchprev();
                    goto redraw;
                }
                if(cmdidx>0)
                    cmdidx--;
                goto getcmd;
            }
            // Right
            case 333: {
                if(cmdlast==0) {
                    dbg_switchnext();
                    goto redraw;
                }
                if(cmdidx<cmdlast)
                    cmdidx++;
                goto getcmd;
            }
            // Backspace
            case 14: {
                if(cmdidx==0)
                    goto getcmd;
                cmdidx--;
                kmemcpy(&cmd[cmdidx],&cmd[cmdidx+1],cmdlast-cmdidx+1);
                cmdlast--;
                goto getcmd;
            }
            // Delete
            case 339: {
                if(cmdidx==cmdlast)
                    goto getcmd;
                kmemcpy(&cmd[cmdidx],&cmd[cmdidx+1],cmdlast-cmdidx+1);
                cmdlast--;
                goto getcmd;
            }
            case 29: {
                dbg_inst = 1-dbg_inst;
                goto redraw;
            }
            // F1
            case 59: {
help:
                fx = kx = (maxx-54)/2; ky = 4; fg=0x9c3c1b; bg=0;
                kprintf(
                    "                                                      \n"
                    " OS/Z " ARCH " Debugger (build " OSZ_BUILD ") \n"
                    "                                                      \n"
                    " Shortcuts                                            \n"
                    "  F1 - this help                                      \n"
                    "  Esc - exit debugger (continue or reboot)            \n"
                    "  Tab - switch panels                                 \n"
                    "  Enter - (with empty command) step instruction       \n"
                    "  Enter - (with command) execute command              \n"
                    "  Left - (with empty command) switch to previous task \n"
                    "  Left - (with command) move cursor                   \n"
                    "  Right - (with empty command) switch to next task    \n"
                    "  Right - (with command) move cursor                  \n"
                    "  Ctrl - toggle instruction disassemble               \n"
                    "                                                      \n"
                    " Commands                                             \n"
                    "  step - step instruction                             \n"
                    "  continue - continue execution                       \n"
                    "  reset, reboot - reboot computer                     \n"
                    "  halt - power off computer                           \n"
                    "  help - this help                                    \n"
                    "  prev - switch to previous task                      \n"
                    "  next - switch to next task                          \n"
                    "  tcb - examine current task's Thread Control Block   \n"
                    "  messages - examine message queue                    \n"
                    "  queues - examine task queues                        \n"
                    "  ram - examine RAM allocation                        \n"
                    "  instruction - toggle instruction disassemble        \n"
                    "  goto X - go to address X                            \n"
                    "                                                      \n"
                );
                kwaitkey();
                goto redraw;
            }
            // Enter
            case 28: {
                /* parse commands */
                // step, continue, exit
                if(cmdlast==0 || cmd[0]=='s' || cmd[0]=='c' || cmd[0]=='e') {
                    if(cmdlast==0 || cmd[0]=='s') {
                        /* TODO: set up debug registers on next instruction */
                        rip = disasm(rip, NULL);
                    }
                    dbg_active = false;
                    ccb.ist1 = __PAGESIZE;
                    return;
                }
                // reset, reboot
                if(cmd[0]=='r' && cmd[1]=='e'){
                    __asm__ __volatile__ ("movb $0xFE, %%al;outb %%al, $0x64;hlt":::);
                }
                // halt
                if(cmd[0]=='h' && cmd[1]!='e'){
                    sys_disable();
                }
                // help
                if(cmd[0]=='h' && cmd[1]=='e'){
                    cmdidx = cmdlast = 0;
                    cmd[cmdidx]=0;
                    goto help;
                }
                // prev
                if(cmd[0]=='p'){
                    cmdidx = cmdlast = 0;
                    cmd[cmdidx]=0;
                    dbg_switchprev();
                    goto redraw;
                }
                // next
                if(cmd[0]=='n'){
                    cmdidx = cmdlast = 0;
                    cmd[cmdidx]=0;
                    dbg_switchnext();
                    goto redraw;
                }
                // tcb
                if(cmd[0]=='t'){
                    cmdidx = cmdlast = 0;
                    cmd[cmdidx]=0;
                    dbg_tab = tab_tcb;
                    goto redraw;
                }
                // messages
                if(cmd[0]=='m'){
                    cmdidx = cmdlast = 0;
                    cmd[cmdidx]=0;
                    dbg_tab = tab_msg;
                    goto redraw;
                }
                // queues
                if(cmd[0]=='q'){
                    cmdidx = cmdlast = 0;
                    cmd[cmdidx]=0;
                    dbg_tab = tab_queues;
                    goto redraw;
                }
                // ram
                if(cmd[0]=='r'){
                    cmdidx = cmdlast = 0;
                    cmd[cmdidx]=0;
                    dbg_tab = tab_ram;
                    goto redraw;
                }
                // instruction
                if(cmd[0]=='i'){
                    cmdidx = cmdlast = 0;
                    cmd[cmdidx]=0;
                    dbg_inst = 1-dbg_inst;
                    goto redraw;
                }
                // goto
                if(cmd[0]=='g'){
                    x=0; while(x<cmdlast && cmd[x]!=' ') x++;
                    if(cmd[x]==' ') {
                        while(x<cmdlast && cmd[x]==' ') x++;
                        if(cmd[x]=='-'||cmd[x]=='+') x++;
                        y = 0;
                        env_hex((unsigned char*)&cmd[x], (uint64_t*)&y, 0, 0);
                        if(y==0) {
                            rip = tcb->rip;
                            cmdidx = cmdlast = 0;
                            cmd[cmdidx]=0;
                        } else {
                            if(cmd[x-1]=='-')
                                rip -= y;
                            else if(cmd[x-1]=='+')
                                rip += y;
                            else
                                rip = y;
                        }
                    } else {
                        rip = disasm(rip, NULL);
                        dbg_start = dbg_next;
                    }
                    /* don't clear the command */
                    goto redraw;
                }
                /* unknown command, do nothing */
                goto getcmd;
            }
            default: {
                if(cmdlast>=sizeof(cmd)-1)
                    break;
                if(cmdidx<cmdlast) {
                    for(x=cmdlast;x>cmdidx;x--)
                        cmd[x]=cmd[x-1];
                }
                if(scancode>=2&&scancode<=13) {
                    c = "1234567890-+"[scancode-2];
                } else
                if(scancode>=16 && scancode<=27) {
                    c = "qwertyuiop[]"[scancode-16];
                } else
                if(scancode>=30 && scancode<=40) {
                    c = "asdfghjkl;'"[scancode-30];
                } else
                if(scancode>=44 && scancode<=53) {
                    c = "zxcvbnm,./"[scancode-44];
                } else
                if(scancode==57) {
                    c = ' ';
                } else
                    c = 0;
                if(c!=0) {
                    cmdlast++;
                    cmd[cmdidx++] = c;
                }
                goto getcmd;
            }
        }
    }
    breakpoint;
    __asm__ __volatile__ ("movb $0xFE, %%al;outb %%al, $0x64;cli;hlt":::);
}

#endif
