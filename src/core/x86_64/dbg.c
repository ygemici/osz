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

//breakpoint types
#define DBG_MODE_EXEC 0b00
#define DBG_MODE_READ 0b11
#define DBG_MODE_WRITE 0b01
#define DBG_MODE_PORT 0b10

// external resources
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
extern uint8_t sys_fault;
extern uint64_t lastsym;
extern char *addr_base;

extern uchar *service_sym(virt_t addr);
extern void kprintf_putchar(int c);
extern void kprintf_unicodetable();
extern unsigned char *env_hex(unsigned char *s, uint64_t *v, uint64_t min, uint64_t max);
extern virt_t disasm(virt_t rip, char *str);
extern char *sprintf(char *dst,char* fmt, ...);

// variables
uint8_t __attribute__ ((section (".data"))) dbg_enabled;
uint8_t __attribute__ ((section (".data"))) dbg_active;
uint8_t __attribute__ ((section (".data"))) dbg_tab;
uint8_t __attribute__ ((section (".data"))) dbg_inst;
uint8_t __attribute__ ((section (".data"))) dbg_full;
uint8_t __attribute__ ((section (".data"))) dbg_unit;
uint8_t __attribute__ ((section (".data"))) dbg_numbrk;
uint8_t __attribute__ ((section (".data"))) dbg_iskbd;
uint8_t __attribute__ ((section (".data"))) dbg_isshft;
uint8_t __attribute__ ((section (".data"))) dbg_indump;
uint8_t __attribute__ ((section (".data"))) dbg_tui;
uint64_t __attribute__ ((section (".data"))) dbg_scr;
uint64_t __attribute__ ((section (".data"))) cr2;
uint64_t __attribute__ ((section (".data"))) cr3;
uint64_t __attribute__ ((section (".data"))) dr6;
char __attribute__ ((section (".data"))) dbg_instruction[128];
virt_t __attribute__ ((section (".data"))) dbg_comment;
virt_t __attribute__ ((section (".data"))) dbg_next;
virt_t __attribute__ ((section (".data"))) dbg_start;
virt_t __attribute__ ((section (".data"))) dbg_lastrip;
char __attribute__ ((section (".data"))) *dbg_err;
uint32_t __attribute__ ((section (".data"))) *dbg_theme;
uint32_t __attribute__ ((section (".data"))) theme_panic[] = {0x100000,0x400000,0x800000,0x9c3c1b,0xcc6c4b,0xec8c6b} ;
uint32_t __attribute__ ((section (".data"))) theme_debug[] = {0x000020,0x000040,0x101080,0x3e32a2,0x7c71da,0x867ade} ;
//uint32_t __attribute__ ((section (".data"))) theme_debug[] = {0x000010,0x000020,0x000040,0x1b3c9c,0x4b6ccc,0x6b8cec} ;
char __attribute__ ((section (".data"))) *brk = "BRK? ?? set at ????????????????h";

// different kind of tabs
enum {
    tab_code,
    tab_data,
    tab_msg,
    tab_tcb,
    tab_queues,
    tab_ram,

    tab_last
};

// priority queue names
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

void dbg_putchar(int c)
{
    if(c<8 || c>255 || (dbg_indump && (c<' '||c>=127)) || (c>=127&&c<160) || c==173)
        c='.';
    __asm__ __volatile__ (
        "movl %0, %%eax;movb %%al, %%bl;outb %%al, $0xe9;"
        "movw $0x3fd,%%dx;1:inb %%dx, %%al;andb $0x40,%%al;jz 1b;"
        "subb $5,%%dl;movb %%bl, %%al;outb %%al, %%dx"
    ::"r"(c):"%rax","%rbx","%rdx");
}

void dbg_settheme()
{
    dbg_putchar(27);
    dbg_putchar('[');
    dbg_putchar((fg==0xFFDD33||fg==dbg_theme[4]||fg==dbg_theme[5])?'1':'0');
    dbg_putchar(';');
    dbg_putchar('3');
    if(fg==0xFFDD33) {
        dbg_putchar('3');
    } else if(fg==0x800000) {
        dbg_putchar('1');
    } else {
        if(fg==dbg_theme[3]||fg==dbg_theme[4]) {
            dbg_putchar(dbg_theme==theme_debug?'6':(fg==dbg_theme[3]?'7':'1'));
        } else
        if(fg==dbg_theme[0]||fg==dbg_theme[1]||fg==dbg_theme[2]) {
            dbg_putchar(dbg_theme==theme_debug?'4':'1');
        } else
            dbg_putchar('7');
    }
    if(bg==dbg_theme[2]) {
        dbg_putchar(';');
        dbg_putchar('4');
        dbg_putchar(dbg_theme==theme_debug?'4':'1');
    }
    dbg_putchar('m');
}

void dbg_setbrk(virt_t addr, uint8_t mode, uint8_t len)
{
    dbg_err = brk;
    brk[3]='0'+(dbg_numbrk%4);
    brk[5]=(mode==DBG_MODE_EXEC?'x':(mode==DBG_MODE_READ?'r':(mode==DBG_MODE_WRITE?'w':'p')));
    brk[6]=(len==0b00?'b':(len==0b01?'w':(len==0b11?'d':'q')));
    sprintf(brk+15,"%8x",addr);
    switch(dbg_numbrk%4){
        case 0:
            __asm__ __volatile__ (
                "movq %0, %%rax;movq %1, %%rdx;movq %%rax, %%dr0;"
                "movq %%dr7, %%rax;andq %%rdx,%%rax;movq %%rax, %%dr7"
            ::"r"(addr),"r"((uint64_t)(((len&3)<<18)|((mode&3)<<16)|(addr==0?0:0b11))):"%rax","%rdx");
            break;
        case 1:
            __asm__ __volatile__ (
                "movq %0, %%rax;movq %1, %%rdx;movq %%rax, %%dr1;"
                "movq %%dr7, %%rax;andq %%rdx,%%rax;movq %%rax, %%dr7"
            ::"r"(addr),"r"((uint64_t)(((len&3)<<18)|((mode&3)<<16)|(addr==0?0:0b1100))):"%rax","%rdx");
            break;
        case 2:
            __asm__ __volatile__ (
                "movq %0, %%rax;movq %1, %%rdx;movq %%rax, %%dr2;"
                "movq %%dr7, %%rax;andq %%rdx,%%rax;movq %%rax, %%dr7"
            ::"r"(addr),"r"((uint64_t)(((len&3)<<18)|((mode&3)<<16)|(addr==0?0:0b110000))):"%rax","%rdx");
            break;
        case 3:
            __asm__ __volatile__ (
                "movq %0, %%rax;movq %1, %%rdx;movq %%rax, %%dr3;"
                "movq %%dr7, %%rax;andq %%rdx,%%rax;movq %%rax, %%dr7"
            ::"r"(addr),"r"((uint64_t)(((len&3)<<18)|((mode&3)<<16)|(addr==0?0:0b11000000))):"%rax","%rdx");
        default: break;
    }
    dbg_numbrk++;
}

// set terminal's cursor poisition in tui mode
void dbg_setpos()
{
    if(dbg_tui) {
        dbg_putchar(27);
        dbg_putchar('[');
        if(ky>98)
            dbg_putchar('0'+(((ky+1)/100)%10));
        dbg_putchar('0'+(((ky+1)/10)%10));
        dbg_putchar('0'+((ky+1)%10));
        dbg_putchar(';');
        if(kx>98)
            dbg_putchar('0'+(((kx+1)/100)%10));
        dbg_putchar('0'+(((kx+1)/10)%10));
        dbg_putchar('0'+((kx+1)%10));
        dbg_putchar('H');
        dbg_settheme();
    }
}

// dump a Thread Control Block
void dbg_tcb()
{
    OSZ_tcb *tcb = (OSZ_tcb*)0;
    char *states[] = { "hybernated", "blocked", "running" };

    fg=dbg_theme[4];
    if(dbg_tui)
        dbg_settheme();
    kprintf("[Thread Control Block]\n");
    fg=dbg_theme[3];
    if(dbg_tui)
        dbg_settheme();
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
        fg=dbg_theme[4];
        if(dbg_tui)
            dbg_settheme();
        kprintf("\n[Hybernation info]\n");
        fg=dbg_theme[3];
        if(dbg_tui)
            dbg_settheme();
        kprintf("swapped to: %x\n", tcb->swapminor);
    } else {
        kprintf("\nerrno: %d, exception error code: %d\n", tcb->errno, tcb->excerr);
        fg=dbg_theme[4];
        if(dbg_tui)
            dbg_settheme();
        kprintf("\n[Memory]\n");
        fg=dbg_theme[3];
        if(dbg_tui)
            dbg_settheme();
        kprintf("memory root: %x, allocated pages: %d, linked pages: %d\n", tcb->memroot, tcb->allocmem, tcb->linkmem);
        fg=dbg_theme[4];
        if(dbg_tui)
            dbg_settheme();
        kprintf("\n[Scheduler]\n");
        fg=dbg_theme[3];
        if(dbg_tui)
            dbg_settheme();
        kprintf("user ticks: %d, system ticks: %d, blocked ticks: %d\n", tcb->billcnt, tcb->syscnt, tcb->blkcnt);
        kprintf("next task in queue: %4x, previous task in queue: %4x\n", tcb->next, tcb->prev);
        if(TCB_STATE(tcb->state)==tcb_state_blocked) {
            kprintf("next task in alarm queue: %4x, alarm at: %d.%d\nblocked since: %d", tcb->alarm, tcb->alarmsec, tcb->alarmns, tcb->blktime);
        }
        kprintf("\n");
    }
}

// disassembly instructions and dump registers, stack
void dbg_code(uint64_t rip, uint64_t rs)
{
    OSZ_tcb *tcb = (OSZ_tcb*)0;
    uchar *symstr;
    virt_t ptr, dmp, lastsymsave;
    int i=0;
    uint64_t j, *rsp=(uint64_t*)(rs!=0?rs:tcb->rsp);

    /* registers and stack dump */
    if(!dbg_full) {
        fg=dbg_theme[4];
        if(dbg_tui)
            dbg_settheme();
        kprintf("[Registers]\n");
        fg=dbg_theme[3];
        if(dbg_tui)
            dbg_settheme();
        kprintf("cs=%4x rip=%8x \n",tcb->cs, tcb->rip);
        kprintf("rflags=%8x excerr=%4x \n",tcb->rflags, tcb->excerr);
        kprintf("cr2=%8x cr3=%8x \n\n",cr2, tcb->memroot);
        kprintf("rax=%8x rbx=%8x \n",*((uint64_t*)(tcb->gpr+0)),  *((uint64_t*)(tcb->gpr+8)));
        kprintf("rcx=%8x rdx=%8x \n",*((uint64_t*)(tcb->gpr+16)), *((uint64_t*)(tcb->gpr+24)));
        kprintf("rsi=%8x rdi=%8x \n",*((uint64_t*)(tcb->gpr+32)), *((uint64_t*)(tcb->gpr+40)));
        kprintf(" r8=%8x  r9=%8x \n",*((uint64_t*)(tcb->gpr+48)), *((uint64_t*)(tcb->gpr+56)));
        kprintf("r10=%8x r11=%8x \n",*((uint64_t*)(tcb->gpr+64)), *((uint64_t*)(tcb->gpr+72)));
        kprintf("r12=%8x r13=%8x \n",*((uint64_t*)(tcb->gpr+80)), *((uint64_t*)(tcb->gpr+88)));
        kprintf("r14=%8x r15=%8x \n",*((uint64_t*)(tcb->gpr+96)), *((uint64_t*)(tcb->gpr+104)));
        kprintf("rbp=%8x rsp=%8x \n",*((uint64_t*)(tcb->gpr+112)),tcb->rsp);

        /* stack */
        ky=3; fx = kx = maxx-44;
        if(dbg_tui) {
            dbg_setpos();
        } else {
            dbg_putchar(13);
            dbg_putchar(10);
        }
        fg=dbg_theme[4];
        if(dbg_tui)
            dbg_settheme();
        kprintf("[Stack %8x]\n",rsp);
        fg=dbg_theme[3];
        if(dbg_tui)
            dbg_settheme();
        sys_fault = false;
        while(i++<11 && !sys_fault && ((uint64_t)rsp<(uint64_t)TEXT_ADDRESS||(uint64_t)rsp>(uint64_t)&__bss_start)){
            kprintf("rsp+%1x rbp-%1x: %8x \n",
                !dbg_inst?((uint64_t)rsp)&0xFF:(uint32_t)(uint64_t)rsp - (uint64_t)tcb->rsp,
                (uint32_t)((uint64_t)(tcb->gpr+112)-(uint64_t)rsp),
                *rsp
            );
            rsp++;
        }
        if(dbg_tui) {
            dbg_setpos();
        }
        fg = dbg_theme[2];
        if(dbg_tui)
            dbg_settheme();
        kprintf(sys_fault?"  * not mapped %d *\n":"  * end *\n", sys_fault);
        fg = dbg_theme[3];
        if(dbg_tui)
            dbg_settheme();

        /* back trace */
        ky=16; kx=fx=2;
        fg=dbg_theme[4];
        if(dbg_tui)
            dbg_setpos();
        rsp = (uint64_t*)(ccb.ist3+ISR_STACK-40);
        kprintf("\n[Back trace %8x]\n", rsp);
        if(dbg_lastrip) {
            fg=dbg_theme[3];
            if(dbg_tui)
                dbg_settheme();
            kprintf("%8x: %s \n",
                dbg_lastrip, service_sym(dbg_lastrip)
            );
        }
        fg=0xFFDD33;
        if(dbg_tui)
            dbg_settheme();
        kprintf("%8x: %s \n",
            rip, service_sym(rip)
        );
        fg=dbg_theme[3];
        if(dbg_tui)
            dbg_settheme();
        sys_fault = false;
        i=0;
        while(i++<4 && !sys_fault && rsp!=0 && ((uint64_t)rsp<(uint64_t)TEXT_ADDRESS||(uint64_t)rsp>(uint64_t)&__bss_start)){
            if((((rsp[1]==0x23||rsp[1]==0x08) &&
                (rsp[4]==0x1b||rsp[4]==0x18))) && !sys_fault) {
                kprintf("%8x: %s   * interrupt %x * \n",*rsp, service_sym(*rsp), rsp[3]);
                rsp=(uint64_t*)rsp[3];
                continue;
            }
            if(sys_fault)
                break;
            if((*rsp>TEXT_ADDRESS && *rsp<BSS_ADDRESS) ||
               (*rsp>((uint64_t)&__bss_start))) {
                if(sys_fault)
                    break;
                symstr = service_sym(*rsp);
                if(sys_fault)
                    break;
                if((virt_t)symstr>(virt_t)TEXT_ADDRESS &&
                    (virt_t)symstr<(virt_t)BSS_ADDRESS && kstrcmp(symstr,"_edata"))
                    kprintf("%8x: %s \n", *rsp, symstr);
            }
            rsp++;
        }
        kprintf("\n");
    }

    /* disassemble block */
    kx=fx=2;
    fg=dbg_theme[4];
    if(dbg_tui)
        dbg_settheme();
    sys_fault = false;
    symstr = service_sym(rip);
    if(sys_fault) {
        kprintf("[Code %x: unknown]\n", rip);
        sys_fault = false;
        return;
    } else
        kprintf("[Code %x: %s +%2x]\n", rip, symstr, rip-lastsym);
    /* disassemble instructions */
    fg=dbg_theme[3];
    if(dbg_tui)
        dbg_settheme();
    /* find a start position */
    if(dbg_lastrip && dbg_lastrip > rip)
        dbg_start = rip-15;
    if(dbg_start == rip) {
        dbg_next = dbg_start;
        do {
            dbg_start--;
        } while(dbg_start>dbg_next-15 && disasm(dbg_start,NULL)!=dbg_next);
    }
    if(dbg_start<lastsym)
        dbg_start = lastsym;
    j = dbg_start && dbg_start > rip-63 && dbg_start <= rip ? dbg_start :
        (dbg_lastrip && dbg_lastrip > rip-63 && dbg_lastrip <= rip ? dbg_lastrip :
        rip-15);
    if(dbg_lastrip && dbg_lastrip < j && dbg_lastrip > j-15) {
        j = dbg_lastrip-15;
    }
    if(lastsym && j < lastsym)
        j = lastsym;
    i = 0;
    while(!i && j<=rip) {
        ptr = j;
        sys_fault = false;
        while(ky < maxy-2 && !sys_fault) {
            if(ptr == rip) {
                i = 1;
                ptr = j;
                break;
            }
            ptr = disasm((virt_t)ptr, NULL);
        }
        j++;
    }
    dbg_start = ptr;
    dbg_next = disasm(dbg_start, NULL);
    while(ky < maxy-2) {
        fg= ptr==rip?0xFFDD33:dbg_theme[3];
        if(dbg_tui)
            dbg_settheme();
        kprintf("%2x %c%c ",!dbg_inst?((uint64_t)ptr)&0xffffffffffff:(uint64_t)ptr-lastsym,ptr==rip||ptr==dbg_lastrip?
            ((dbg_lastrip<rip?dbg_lastrip:rip)==ptr?(dbg_lastrip==rip?'-':'/'):'\\')
            :(
            (dbg_lastrip<rip && ptr>dbg_lastrip && ptr<rip)||
            (dbg_lastrip>rip && ptr>rip && ptr<dbg_lastrip)?'|':' '),
            ptr==rip?'>':(ptr==dbg_lastrip?'-':' '));
        dbg_comment = 0;
        dmp = ptr;
        lastsymsave = lastsym;
        sys_fault = false;
        ptr = disasm((virt_t)ptr, dbg_instruction);
        if(sys_fault) {
            fg = dbg_theme[2];
            if(dbg_tui)
                dbg_settheme();
            kprintf(" * not mapped %d *", sys_fault);
            fg = dbg_theme[3];
            if(dbg_tui)
                dbg_settheme();
            return;
        }
        if(dbg_inst) {
            kprintf("%s", dbg_instruction);
        } else {
            for(;dmp<ptr;dmp++)
                kprintf("%1x ",*((uchar*)dmp));
        }
        if(dbg_comment) {
            fg=dbg_theme[2];
            kx=maxx/2+10;
            sys_fault = false;
            if(addr_base == NULL){
                if(dbg_comment<__PAGESIZE) {
                    symstr = (uchar*)"tcb";
                    lastsym = 0;
                } else
                if(dbg_comment>=__PAGESIZE && dbg_comment<__SLOTSIZE/2) {
                    symstr = (uchar*)"mq";
                    lastsym = __PAGESIZE;
                } else
                if(dbg_comment>=__SLOTSIZE/2 && dbg_comment<__SLOTSIZE) {
                    symstr = (uchar*)"stack";
                    lastsym = __SLOTSIZE/2;
                } else
                    symstr = service_sym(dbg_comment);
            }
            if(sys_fault && *((uchar*)&dbg_comment)>=32 && *((uchar*)&dbg_comment)<127 &&
                (dbg_comment&0xFF00000000000000)==0) {
                sys_fault=0;
                symstr = (uchar*)&dbg_comment;
            }
            if(!sys_fault && symstr!=NULL && symstr[0]!='(') {
                if(dbg_tui)
                    dbg_setpos();
                else {
                    dbg_putchar(9);
                    dbg_putchar(9);
                    dbg_putchar(';');
                }
                if(symstr != (uchar*)&dbg_comment)
                    kprintf("%s +%x", symstr, (uint32_t)(dbg_comment-lastsym));
                else
                    kprintf("'%s'", symstr);
            }
        }
        lastsym = lastsymsave;
        kprintf("\n");
    }
}

// dump memory in bytes, words, double words or quad words
void dbg_data(uint64_t ptr)
{
    int i,j;
    ky = 3;
    if(dbg_tui)
        dbg_setpos();
    while(ky<maxy-2) {
        kx=1;
        kprintf("%8x: ",ptr);
        sys_fault = false;
        __asm__ __volatile__ ("movq %0, %%rdi;movb (%%rdi), %%al"::"m"(ptr):"%rax","%rdi");
        if(sys_fault) {
            fg = dbg_theme[2];
            kprintf(" * not mapped %d *", sys_fault);
            fg = dbg_theme[3];
        }
        dbg_indump = true;
        switch(dbg_unit) {
            // stack view
            case 4:
                if(!sys_fault) {
                    kx = 19; kprintf("%8x ",*((uint64_t*)ptr));
                    kx = 70;
                    kprintf_putchar(*((uint8_t*)ptr)); kx++;
                    kprintf_putchar(*((uint8_t*)(ptr+1))); kx++;
                    kprintf_putchar(*((uint8_t*)(ptr+2))); kx++;
                    kprintf_putchar(*((uint8_t*)(ptr+3))); kx++;
                    kprintf_putchar(*((uint8_t*)(ptr+4))); kx++;
                    kprintf_putchar(*((uint8_t*)(ptr+5))); kx++;
                    kprintf_putchar(*((uint8_t*)(ptr+6))); kx++;
                    kprintf_putchar(*((uint8_t*)(ptr+7)));
                }
                ptr+=8;
                break;
            // qword
            case 3:
                for(i=0;i<2;i++) {
                    if(!sys_fault) {
                        kx = 17*i+19; kprintf("%8x ",*((uint64_t*)ptr));
                    }
                    ptr += 8;
                }
                ptr -= 16;
                for(i=0;i<2;i++) {
                    if(!sys_fault) {
                        kx = 9*i+70;
                        kprintf_putchar(*((uint8_t*)(ptr+7))); kx++;
                        kprintf_putchar(*((uint8_t*)(ptr+6))); kx++;
                        kprintf_putchar(*((uint8_t*)(ptr+5))); kx++;
                        kprintf_putchar(*((uint8_t*)(ptr+4))); kx++;
                        kprintf_putchar(*((uint8_t*)(ptr+3))); kx++;
                        kprintf_putchar(*((uint8_t*)(ptr+2))); kx++;
                        kprintf_putchar(*((uint8_t*)(ptr+1))); kx++;
                        kprintf_putchar(*((uint8_t*)ptr));
                        dbg_putchar(' ');
                    }
                    ptr += 8;
                }
                break;
            // dword
            case 2:
                for(i=0;i<4;i++) {
                    if(!sys_fault) {
                        kx = 9*i+19; kprintf("%4x ",*((uint32_t*)ptr));
                    }
                    ptr += 4;
                }
                ptr -= 16;
                for(i=0;i<4;i++) {
                    if(!sys_fault) {
                        kx = 5*i+68;
                        kprintf_putchar(*((uint8_t*)(ptr+3))); kx++;
                        kprintf_putchar(*((uint8_t*)(ptr+2))); kx++;
                        kprintf_putchar(*((uint8_t*)(ptr+1))); kx++;
                        kprintf_putchar(*((uint8_t*)ptr));
                        dbg_putchar(' ');
                    }
                    ptr += 4;
                }
                break;
            // word
            case 1:
                j=18;
                for(i=0;i<8;i++) {
                    if(i%4==0) j++;
                    if(!sys_fault) {
                        kx = 5*i+j; kprintf("%2x ",*((uint16_t*)ptr));
                    }
                    ptr += 2;
                }
                ptr -= 16;
                for(i=0;i<8;i++) {
                    if(!sys_fault) {
                        kx = 3*i+64;
                        kprintf_putchar(*((uint8_t*)(ptr+1))); kx++;
                        kprintf_putchar(*((uint8_t*)ptr));
                        dbg_putchar(' ');
                    }
                    ptr += 2;
                }
                break;
            // byte
            default:
                j=18;
                for(i=0;i<16;i++) {
                    if(i%4==0) {
                        dbg_putchar(' ');
                        j++;
                    }
                    if(!sys_fault) {
                        kx = 3*i+j;
                        kprintf("%1x ",*((uint8_t*)ptr));
                    }
                    ptr++;
                }
                ptr -= 16;
                for(i=0;i<16;i++) {
                    if(!sys_fault) {
                        kx = i+71; kprintf_putchar(*((uint8_t*)ptr));
                    }
                    ptr++;
                }
                break;
        }
        dbg_indump = false;
        kprintf("\n");
    }
}

// list messages in Thread Message Queue
void dbg_msg()
{
    uint64_t m, i, t;

    fg=dbg_theme[4];
    if(dbg_tui)
        dbg_settheme();
    kprintf("[Queue Header]\n");
    fg=dbg_theme[3];
    if(dbg_tui)
        dbg_settheme();
    kprintf("msg in: %3d, msg out: %3d, msg max: %3d, recv from: %x\n",
        *((uint64_t*)(MQ_ADDRESS)), *((uint64_t*)(MQ_ADDRESS+8)),
        *((uint64_t*)(MQ_ADDRESS+16)),*((uint64_t*)(MQ_ADDRESS+24)));
    kprintf("buf in: %3d, buf out: %3d, buf max: %3d, buf min: %3d\n",
        *((uint64_t*)(MQ_ADDRESS+32)), *((uint64_t*)(MQ_ADDRESS+40)),
        *((uint64_t*)(MQ_ADDRESS+48)),*((uint64_t*)(MQ_ADDRESS+56)));
    fg=dbg_theme[4];
    if(dbg_tui)
        dbg_settheme();
    kprintf("\n[Messages]");
    /* print out last message as well */
    i = *((uint64_t*)MQ_ADDRESS) - 1;
    if(i==0)
        i = *((uint64_t*)(MQ_ADDRESS+16)) - 1;
    do {
        t = i==*((uint64_t*)MQ_ADDRESS)?4:(
            *((uint64_t*)MQ_ADDRESS)!=*((uint64_t*)MQ_ADDRESS+8) &&
                (i>=*((uint64_t*)MQ_ADDRESS)||i<*((uint64_t*)MQ_ADDRESS+8))?2:3);
        fg = t==4 ? 0xFFDD33 : dbg_theme[t];
        if(dbg_tui)
            dbg_settheme();
        m = ((i * sizeof(msg_t))+(uint64_t)MQ_ADDRESS);
        kprintf("\n %2x: %8x %8x ", m,
            *((uint64_t*)m), *((uint64_t*)(m+8)));
        kprintf("%8x %8x\n %s",
            *((uint64_t*)(m+16)), *((uint64_t*)(m+24)),
            t==4?"next":(t==3?"    ":"done"));
        kprintf("  %8x %8x %8x %8x\n",
            *((uint64_t*)(m+32)), *((uint64_t*)(m+40)),
            *((uint64_t*)(m+48)), *((uint64_t*)(m+56)));
        i++;
        if(i>=*((uint64_t*)(MQ_ADDRESS+16))-1)
            i=1;
    } while (i!=*((uint64_t*)(MQ_ADDRESS+8)) && ky<maxy-3);
}

// dump CPU Control Block and priority queues
void dbg_queues()
{
    OSZ_tcb *tcbq = (OSZ_tcb*)&tmpmap;
    int i, j;
    pid_t n;

    fg=dbg_theme[4];
    if(dbg_tui)
        dbg_settheme();
    kprintf("[CPU Control Block]\n");
    fg=dbg_theme[3];
    if(dbg_tui)
        dbg_settheme();
    kprintf("Cpu real id: %d, logical id: %d, ",ccb.realid, ccb.id, ccb.ist1, ccb.ist2);
    kprintf("last xreg: %4x, mutex: %4x%4x%4x\n", ccb.lastxreg, ccb.mutex[0], ccb.mutex[1], ccb.mutex[2]);
    fg=dbg_theme[4];
    if(dbg_tui)
        dbg_settheme();
    kprintf("\n[Blocked Task Queues]\n");
    fg=dbg_theme[3];
    if(dbg_tui)
        dbg_settheme();
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
    fg=dbg_theme[4];
    if(dbg_tui)
        dbg_settheme();
    kprintf("\n[Active Task Queues]\n No Queue     Head      Current   #Tasks\n");
    fg=dbg_theme[3];
    if(dbg_tui)
        dbg_settheme();
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

// show info on Physical Memory
void dbg_ram()
{
    OSZ_pmm_entry *fmem = pmm.entries;
    int i = pmm.size;

    fg=dbg_theme[4];
    if(dbg_tui)
        dbg_settheme();
    if(!dbg_full) {
        kprintf("[Physical Memory Manager]\n");
        fg=dbg_theme[3];
        if(dbg_tui)
            dbg_settheme();
        kprintf("Core bss: %8x - %8x, %d pages\nNumber of free memory fragments: %d\n\n",
            pmm.bss, pmm.bss_end,(pmm.bss_end-pmm.bss)/__PAGESIZE,pmm.size
        );
        kprintf("Total: %9d pages, allocated: %9d pages, free: %9d pages\n",
            pmm.totalpages, pmm.totalpages-pmm.freepages, pmm.freepages
        );
        dbg_putchar('[');
        bg = fg = dbg_theme[2];
        if(dbg_tui)
            dbg_settheme();
        for(i=0;i<(pmm.totalpages-pmm.freepages)*(maxx-9)/(pmm.totalpages+1);i++) {
            kprintf_putchar('#'); kx++;
        }
        bg = fg = dbg_theme[0];
        if(dbg_tui)
            dbg_settheme();
        for(;i<(maxx-9);i++) {
            kprintf_putchar('.'); kx++;
        }
        fg = dbg_theme[3];
        bg = dbg_theme[1];
        if(dbg_tui)
            dbg_settheme();
        dbg_putchar(']');
        kprintf(" %2d.%d%%\n\n",
            (pmm.totalpages-pmm.freepages)*100/(pmm.totalpages+1), ((pmm.totalpages-pmm.freepages)*1000/(pmm.totalpages+1))%10
        );
        fg=dbg_theme[4];
        if(dbg_tui)
            dbg_settheme();
    }
    kprintf("[Free Memory Fragments]\nAddress           Num pages\n");
    fg=dbg_theme[3];
    if(dbg_tui)
        dbg_settheme();
    fmem += dbg_scr;
    for(i=dbg_scr;i<pmm.size&&ky<maxx-2;i++) {
        kprintf("%8x, %9d\n",fmem->base, fmem->size);
        fmem++;
    }
}

// switch to the next task
void dbg_switchprev(virt_t *rip, virt_t *rsp)
{
    OSZ_tcb *tcb = (OSZ_tcb*)0;
    OSZ_tcb *tcbq = (OSZ_tcb*)&tmpmap;
    pid_t p = tcb->prev;
    int i;

    if(p == 0) {
        if(TCB_STATE(tcb->state) == tcb_state_blocked) {
            for(i=PRI_IDLE;i>0 && !p;i--)
                p = ccb.hd_active[i];
        } else {
            /* find the next non empty higher priority queue */
            if(tcb->priority>0)
                for(i=tcb->priority-1;i>=0 && !p;i--)
                    p = ccb.hd_active[i];
            if(!p)
                p = ccb.hd_blocked;
            /* find the last non empty queue */
            if(!p)
                for(i=PRI_IDLE;i>0 && !p;i--)
                    p = ccb.hd_active[i];
        }
        /* find the last entry in the list */
        do {
            kmap((virt_t)&tmpmap, p * __PAGESIZE, PG_CORE_NOCACHE);
            p = tcbq->next;
        } while(p != 0);
        p = tcbq->mypid;
    }
    if(p) {
        kmap((virt_t)&tmpmap, p * __PAGESIZE, PG_CORE_NOCACHE);
        tcb->rip = *rip;
        thread_map(tcbq->memroot);
        *rip = tcb->rip;
        *rsp = tcb->rsp;
    }
}

// switch to the previous task
void dbg_switchnext(virt_t *rip, virt_t *rsp)
{
    OSZ_tcb *tcb = (OSZ_tcb*)0;
    OSZ_tcb *tcbq = (OSZ_tcb*)&tmpmap;
    pid_t n = tcb->next;
    int i;
    if(n == 0) {
        if(TCB_STATE(tcb->state) == tcb_state_blocked) {
            n = ccb.hd_active[PRI_SYS];
        } else {
            n = 0;
            /* find the next non empty lower priority queue */
            if(tcb->priority<7)
                for(i=tcb->priority+1;i<=7 && !n;i++) {
                    n = ccb.hd_active[i];
            }
            if(i>7 || !n)
                n = ccb.hd_blocked ? ccb.hd_blocked : ccb.hd_active[PRI_SYS];
        }
    }
    if(n) {
        kmap((virt_t)&tmpmap, n * __PAGESIZE, PG_CORE_NOCACHE);
        tcb->rip = *rip;
        thread_map(tcbq->memroot);
        *rip = tcb->rip;
        *rsp = tcb->rsp;
    }
}

extern virt_t service_lookupsym(uchar *sym, size_t size);

virt_t dbg_getaddr(char *cmd, size_t size)
{
    uint64_t base = 0,ret = 0;
    char *s=cmd;
    if(*cmd<'0'||*cmd>'9') {
        while((uint64_t)cmd < (uint64_t)s+size &&
            *cmd != ' ' && *cmd != '-' && *cmd != '+'
        )
            cmd++;
        base = service_lookupsym((uchar*)s,(uint64_t)cmd-(uint64_t)s);
    }
    while(*cmd==' '||*cmd=='-'||*cmd=='+') cmd++;
    env_hex((unsigned char*)cmd, (uint64_t*)&ret, 0, 0);
    return (*(cmd-1)=='-'?base-ret:base+ret);
}

#define SINGLESTEP_NONE 0
#define SINGLESTEP_LOCAL 1
#define SINGLESTEP_GLOBAL 2

void dbg_singlestep(uint8_t enable)
{
    OSZ_tcb *tcb=(OSZ_tcb*)&tmpmap;
    int i;
    pid_t n;
    // enable globally or clear all flags
    for(i=PRI_SYS; i<PRI_IDLE; i++) {
        if(ccb.hd_active[i] == 0)
            continue;
        n = ccb.hd_active[i];
        while(n != 0) {
            kmap((virt_t)&tmpmap, n * __PAGESIZE, PG_CORE_NOCACHE);
            if(enable==SINGLESTEP_GLOBAL)
                tcb->rflags |= (1<<8);
            else
                tcb->rflags &= ~(1<<8);
            n = tcb->next;
        }
    }
    n = ccb.hd_blocked;
    while(n != 0) {
        kmap((virt_t)&tmpmap, n * __PAGESIZE, PG_CORE_NOCACHE);
        if(enable==SINGLESTEP_GLOBAL)
            tcb->rflags |= (1<<8);
        else
            tcb->rflags &= ~(1<<8);
        n = tcb->next;
    }
    // enable locally
    if(enable) {
        *((uint64_t*)(ccb.ist3+ISR_STACK-24)) |= (1<<8);
        ((OSZ_tcb*)0)->rflags |= (1<<8);
    } else {
        *((uint64_t*)(ccb.ist3+ISR_STACK-24)) &= ~(1<<8);
        ((OSZ_tcb*)0)->rflags &= ~(1<<8);
    }
}

// initialize debugger. Called from sys_enable()
void dbg_init()
{
    dbg_enabled = 1;
    dbg_err = NULL;
    dbg_theme = theme_debug;
    dbg_unit = 0;
    dbg_start = 0;
    dbg_lastrip = 0;
    dbg_tab = tab_code;
    dbg_inst = 1;
    dbg_full = 0;
    dbg_scr = 0;
    __asm__ __volatile__ (
        "movw $0x3f9, %%dx;"
        "xorb %%al, %%al;outb %%al, %%dx;"   //int off
        "movb $0x80, %%al;addb $2,%%dl;outb %%al, %%dx;"  //set divisor mode
        "movb $12, %%al;subb $3,%%dl;outb %%al, %%dx;"    //lo 9600
        "xorb %%al, %%al;incb %%dl;outb %%al, %%dx;"   //hi
        "xorb %%al, %%al;incb %%dl;outb %%al, %%dx;"   //fifo off
        "movb $3, %%al;incb %%dl;outb %%al, %%dx;"     //8N1
        "movb $0x4, %%al;incb %%dl;outb %%al, %%dx;"   //mcr
        "xorb %%al, %%al;subb $4,%%dl;outb %%al, %%dx;inb %%dx, %%al"   //clear receiver/transmitter
    :::"%rax","%rdx");
}

// activate debugger. Called from ISRs
void dbg_enable(virt_t rip, virt_t rsp, char *reason)
{
    OSZ_tcb *tcb = (OSZ_tcb*)0;
    OSZ_tcb *tcbq = (OSZ_tcb*)&tmpmap;
    uint64_t scancode = 0, offs, line, x, y;
    OSZ_font *font = (OSZ_font*)&_binary_font_start;
    char *tabs[] = { "Code", "Data", "Messages", "TCB", "CCB", "RAM" };
    char cmd[64], c;
    int cmdidx=0,cmdlast=0;
    uint8_t m,l;

    // turn of scroll
    scry = -1;
    // set up variables and stack
    dbg_active = true;
    __asm__ __volatile__ ( "movq %%cr2, %%rax; movq %%rax, %0" : "=r"(cr2) : : "%rax");
    __asm__ __volatile__ ( "movq %%cr3, %%rax; movq %%rax, %0" : "=r"(cr3) : : "%rax");
    __asm__ __volatile__ ( "movq %%dr6, %%rax; movq %%rax, %0;": "=r"(dr6) : : "%rax");
    __asm__ __volatile__ ( "xorq %%rax, %%rax; movq %%rax,%%dr6": : : "%rax");
    ccb.ist1 = (uint64_t)safestack + (uint64_t)__PAGESIZE-1024;

    // get rip and rsp to display
    if(rip == 0)
        rip = tcb->rip;
    if(rsp == 0)
        rsp = tcb->rsp;

    dbg_next = 0;
    dbg_isshft = false;

    if(reason==NULL) {
        if(dr6&(1<<14))
            reason="single step";
        if(dr6&(1<<0)||dr6&(1<<1)||dr6&(1<<2)||dr6&(1<<3))
            reason="breakpoint";
    }
    kprintf_reset();
    if(!dbg_tui) {
        dbg_putchar(13);
        dbg_putchar(10);
        fg = 0xFFDD33;
        bg = 0;
        kprintf("OS/Z debug");
        if(reason!=NULL&&*reason!=0)
            kprintf(": %s  ", reason);
        kprintf("\n");
    } else {
        dbg_putchar(27);
        dbg_putchar('c');
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

    /* redraw and get command */
    while(1) {
redraw:
        fx=kx=ky=0;
        if(dbg_tui) {
            dbg_putchar(27);
            dbg_putchar('[');
            dbg_putchar('0');
            dbg_putchar('m');
            fg = 0xFFDD33;
            bg = 0;
            // clear screen
            dbg_setpos();
            dbg_putchar(27);
            dbg_putchar('[');
            dbg_putchar('2');
            dbg_putchar('J');
            kprintf("OS/Z debug");
            if(reason != NULL && *reason != 0)
                kprintf(": %s  ", reason);
            kprintf("\n");
        } else {
            dbg_putchar(13);
            dbg_putchar(10);
        }
        ky=1;
        for(x=0; x<tab_last; x++) {
            kx++;
            bg = dbg_tab==x ? dbg_theme[1] : 0;
            fg = dbg_tab==x ? dbg_theme[5] : dbg_theme[2];
            if(dbg_tui)
                dbg_settheme();
            kprintf_putchar(' ');
            kx++;
            dbg_putchar(dbg_tab==x?'[':' ');
            kprintf(tabs[x]);
            dbg_putchar(dbg_tab==x?']':' ');
            kprintf_putchar(' ');
            kx++;
        }
        dbg_putchar(13);
        dbg_putchar(10);

        // clear tab
        bg=dbg_theme[1];
        offs = font->height*bootboot.fb_scanline*2;
        for(y=font->height;y<(maxy-2)*font->height;y++){
            line=offs;
            for(x=0;x<bootboot.fb_width;x++){
                *((uint32_t*)(FBUF_ADDRESS + line))=(uint32_t)bg;
                line+=4;
            }
            offs+=bootboot.fb_scanline;
        }
        // draw tab
        fx=kx=2; ky=3; fg = dbg_theme[3];
        if(dbg_tui)
            dbg_setpos();
        __asm__ __volatile__ ("pushq %%rdi":::"%rdi");
        if(dbg_tab==tab_code) dbg_code(rip, tcb->rsp); else
        if(dbg_tab==tab_data) dbg_data(rsp); else
        if(dbg_tab==tab_tcb) dbg_tcb(); else
        if(dbg_tab==tab_msg) dbg_msg(); else
        if(dbg_tab==tab_queues) dbg_queues(); else
        if(dbg_tab==tab_ram) dbg_ram();
        __asm__ __volatile__ ("popq %%rdi":::"%rdi");
getcmd:
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
        fg = 0x404040; bg=0;
        dbg_putchar(13);
        kx = maxx-5 - (tcb->mypid>0xff?(tcb->mypid>0xffff?8:2):1) - kstrlen((char*)tcb->name);
        ky = maxy-1;
        scry = -1;
        if(dbg_tui) {
            dbg_setpos();
            dbg_putchar(27);
            dbg_putchar('[');
            dbg_putchar('2');
            dbg_putchar('K');
        }
        kprintf("%s %x ", tcb->name, tcb->mypid);
        if(dbg_tui)
            dbg_putchar('\r');
        fx=kx=0; ky=maxy-1; fg=0x808080; bg=0;
        if(dbg_tui)
            dbg_settheme();
        cmd[cmdlast]=0;
        kprintf("dbg> %s",cmd);
        if(dbg_err != NULL) {
            fg=0x800000;
            if(dbg_tui)
                dbg_settheme();
            kprintf("  * %s *", dbg_err);
            dbg_err = NULL;
            fg=0x808080;
            if(dbg_tui)
                dbg_settheme();
        }
        fg=0; bg=0x404040;
        kx=5+cmdidx;
        if(dbg_tui)
            dbg_setpos();
        x = cmdidx==cmdlast||cmd[cmdidx]==0?' ':cmd[cmdidx];
        kprintf_putchar(x);
        dbg_putchar(8);
        //failsafe, clear all attributes
        if(dbg_tui) {
            dbg_putchar(27);
            dbg_putchar('[');
            dbg_putchar('0');
            dbg_putchar('m');
        }
/*
        if(scancode) {
            kx=maxx-4; fg=0x101010; bg=0;
            kprintf("%d",scancode);
        }
*/
        scancode = kwaitkey();
        /* translate ANSI terminal codes */
        if(dbg_iskbd==false) {
            if(scancode==12)
                goto redraw;
            else
            if(scancode==27) {
                scancode = kwaitkey();
                if(scancode==27)
                    scancode=1;
                else
                if(scancode=='[') {
                    scancode = kwaitkey();
                    if(scancode=='A')
                        scancode=328;
                    else
                    if(scancode=='B')
                        scancode=336;
                    else
                    if(scancode=='C')
                        scancode=333;
                    else
                    if(scancode=='D')
                        scancode=331;
                    else
                    if(scancode=='3') {
                        scancode = kwaitkey();
                        if(scancode=='~')
                            scancode=339;
                    }
                    if(scancode=='1') {
                        scancode = kwaitkey();
                        if(scancode=='1') {
                            scancode = kwaitkey();
                            if(scancode=='~')
                                scancode=59;
                        }
                        if(scancode=='2') {
                            scancode = kwaitkey();
                            if(scancode=='~')
                                scancode=60;
                        }
                    }
                }
            } else
            if(scancode==3) {
                scancode=1;
            } else
            if(scancode==8||scancode==127)
                scancode=14;
            else
            if(scancode=='\t')
                scancode=15;
            else
            if(scancode=='\r'||scancode=='\n')
                scancode=28;
        }
        /* translate scancodes */
        switch(scancode){
            // ESC
            case 1: {
                cmdidx = cmdlast = 0;
                cmd[cmdidx]=0;
                goto getcmd;
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
                    dbg_switchprev(&rip, &rsp);
                    goto redraw;
                }
                if(cmdidx>0 && (dbg_iskbd || dbg_tui))
                    cmdidx--;
                goto getcmd;
            }
            // Right
            case 333: {
                if(cmdlast==0) {
                    dbg_switchnext(&rip, &rsp);
                    goto redraw;
                }
                if(cmdidx<cmdlast && (dbg_iskbd || dbg_tui))
                    cmdidx++;
                goto getcmd;
            }
            // Up
            case 328: {
                if(dbg_tab == tab_data) {
                    rsp -= (dbg_unit==4? 8 : 16);
                    goto redraw;
                } else
                if(dbg_tab == tab_code) {
                    dbg_next = dbg_start;
                    do {
                        dbg_start--;
                    } while(dbg_start>dbg_next-31 && disasm(dbg_start,NULL)!=dbg_next);
                    goto redraw;
                } else
                if(dbg_tab == tab_ram) {
                    if(dbg_scr>0)
                        dbg_scr--;
                    goto redraw;
                }
                goto getcmd;
            }
            // Down
            case 336: {
                if(dbg_tab == tab_data) {
                    rsp += (dbg_unit==4? 8 : 16);
                    goto redraw;
                } else
                if(dbg_tab == tab_code) {
                    dbg_start = dbg_next;
                    goto redraw;
                } else
                if(dbg_tab == tab_ram) {
                    if(pmm.size>0 && dbg_scr<pmm.size-1)
                        dbg_scr++;
                    goto redraw;
                }
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
                dbg_inst = 1 - dbg_inst;
                goto redraw;
            }
            // F1 Help
            case 59: {
help:
                fx = kx = (maxx-54)/2; ky = (maxy-30)/2; fg=dbg_theme[5]; bg=dbg_theme[2];
                if(dbg_tui)
                    dbg_setpos();
                kprintf(
                    "                                                      \n"
                    " OS/Z " ARCH " Debugger (build " OSZ_BUILD ") \n"
                    "                                                      \n"
                    " Keyboard Shortcuts                                   \n"
                    "  F1 - this help                                      \n"
                    "  Tab - switch panels                                 \n"
                    "  Esc - exit debugger / clear command                 \n"
                    "  Enter - step instruction / execute command          \n"
                    "  Left - switch to previous task / move cursor        \n"
                    "  Right - switch to next task / move cursor           \n"
                    "  Ctrl - toggle instruction bytes / mnemonics         \n"
                    "                                                      \n"
                    " Commands                                             \n"
                    "  (none) - repeat last step or continue command       \n"
                    "  Step - step instruction                             \n"
                    "  Continue - continue execution                       \n"
                    "  REset, REboot - reboot computer                     \n"
                    "  Quit, HAlt - power off computer                     \n"
                    "  Help - this help                                    \n"
                    "  Pid X - switch to task                              \n"
                    "  Prev - switch to previous task                      \n"
                    "  Next - switch to next task                          \n"
                    "  Tcb - show current task's Thread Control Block      \n"
                    "  TUi - toggle video terminal support on serial line  \n"
                    "  Messages - list messages in current thread's queue  \n"
                    "  All, CCb - show all task queues and CCB             \n"
                    "  Ram - show RAM information and allocation           \n"
                    "  Instruction, Disasm - instruction disassemble       \n"
                    "  Goto X - go to address X                            \n"
                    "  eXamine [/b1w2d4q8s] X - examine memory at X        \n"
                    "  Break [/b12d4q8rwx] X - set a breakpoint at X       \n"
                    "                                                      \n"
                );
                if(dbg_tui) {
                    dbg_putchar(27);
                    dbg_putchar('[');
                    dbg_putchar('0');
                    dbg_putchar('m');
                }
                if(kwaitkey()==27 && dbg_iskbd==false){
                    while(kwaitkey()!='~');
                }
                goto redraw;
            }
            // F2 UNICODE table
            case 60: {
                fx = kx = (maxx-64)/2; ky = (maxy-33)/2; fg=dbg_theme[5]; bg=dbg_theme[2];
                if(dbg_tui)
                    dbg_setpos();
                else {
                    dbg_putchar(13);
                    dbg_putchar(10);
                }
                dbg_indump = true;
                kprintf_unicodetable();
                dbg_indump = false;
                if(dbg_tui) {
                    dbg_putchar(27);
                    dbg_putchar('[');
                    dbg_putchar('0');
                    dbg_putchar('m');
                }
                if(kwaitkey()==27 && dbg_iskbd==false){
                    while(kwaitkey()!='~');
                }
                goto redraw;
            }
            // Enter
            case 28: {
                /* parse commands */
                // step, continue
                if(cmdlast==0 || cmd[0]=='s' || (cmd[0]=='c' && cmd[1]!='c')) {
                    /* enable or disable single stepping. */
                    if(((dr6>>14)&1) != ((tcb->rflags>>8)&1) || cmdlast!=0) {
                    /* we enable if command starts with 's' or we are
                     * already in single stepping mode */
                        dbg_singlestep((cmdlast==0 && dr6&(1<<14)) || cmd[0]=='s'?
                            (cmd[1]=='g' ? SINGLESTEP_GLOBAL : SINGLESTEP_LOCAL)
                            :SINGLESTEP_NONE
                        );
                    }
                    dbg_start = dbg_next;
                    dbg_lastrip = rip;
                    dbg_active = false;
                    ccb.ist1 = __PAGESIZE;
                    // clear screen
                    offs = 0;
                    for(y=0;y<bootboot.fb_height;y++){
                        line=offs;
                        for(x=0;x<bootboot.fb_width;x+=2){
                            *((uint64_t*)(FBUF_ADDRESS + line))=(uint64_t)0;
                            line+=8;
                        }
                        offs+=bootboot.fb_scanline;
                    }
                    kprintf_reset();
                    scry = -1;
                    __asm__ __volatile__ ( "movq %0, %%rax; movq %%rax, %%cr3" : : "r"(cr3) : "%rax");
                    return;
                } else
                // reset, reboot
                if(cmd[0]=='r' && cmd[1]=='e'){
                    __asm__ __volatile__ ("movb $0xFE, %%al;outb %%al, $0x64;hlt":::);
                } else
                // quit, halt
                if(cmd[0]=='q'||(cmd[0]=='h' && cmd[1]=='a')){
                    sys_disable();
                } else
                // help
                if(cmd[0]=='h'){
                    cmdidx = cmdlast = 0;
                    cmd[cmdidx]=0;
                    goto help;
                } else
                // prev / pid X
                if(cmd[0]=='p'){
                    x=0; while(x<cmdlast && cmd[x]!=' ') x++;
                    if(cmd[x]==' ') {
                        while(x<cmdlast && cmd[x]==' ') x++;
                        y = 0;
                        env_hex((unsigned char*)&cmd[x], (uint64_t*)&y, 0, 0);
                        kmap((virt_t)&tmpmap, y * __PAGESIZE, PG_CORE_NOCACHE);
                        if(tcbq->magic == OSZ_TCB_MAGICH) {
                            tcb->rip = rip;
                            thread_map(tcbq->memroot);
                            rip = tcb->rip;
                            rsp = tcb->rsp;
                        } else {
                            dbg_err = "Invalid pid";
                            goto getcmd;
                        }
                    } else
                        dbg_switchprev(&rip, &rsp);
                    cmdidx = cmdlast = 0;
                    cmd[cmdidx]=0;
                    goto redraw;
                } else
                // next
                if(cmd[0]=='n'){
                    cmdidx = cmdlast = 0;
                    cmd[cmdidx]=0;
                    dbg_switchnext(&rip, &rsp);
                    goto redraw;
                } else
                // tcb
                if(cmd[0]=='t'){
                    cmdidx = cmdlast = 0;
                    cmd[cmdidx]=0;
                    if(cmd[1]=='u')
                        dbg_tui = 1-dbg_tui;
                    else
                        dbg_tab = tab_tcb;
                    goto redraw;
                } else
                // messages
                if(cmd[0]=='m'){
                    cmdidx = cmdlast = 0;
                    cmd[cmdidx]=0;
                    dbg_tab = tab_msg;
                    goto redraw;
                } else
                // all, ccb
                if(cmd[0]=='a' || (cmd[0]=='c'&&cmd[1]=='c')){
                    cmdidx = cmdlast = 0;
                    cmd[cmdidx]=0;
                    dbg_tab = tab_queues;
                    goto redraw;
                } else
                // ram
                if(cmd[0]=='r'){
                    cmdidx = cmdlast = 0;
                    cmd[cmdidx]=0;
                    dbg_tab = tab_ram;
                    goto redraw;
                } else
                // instruction
                if(cmd[0]=='i'){
                    x=0; while(x<cmdlast && cmd[x]!=' ') x++;
                    if(cmd[x]==' ')
                        goto getgoto;
                    cmdidx = cmdlast = 0;
                    cmd[cmdidx]=0;
                    if(dbg_tab != tab_code)
                        dbg_tab = tab_code;
                    else {
                        dbg_inst = 1 - dbg_inst;
                    }
                    goto redraw;
                } else
                // full
                if(cmd[0]=='f'){
                    cmdidx = cmdlast = 0;
                    cmd[cmdidx]=0;
                    dbg_full = 1 - dbg_full;
                    goto redraw;
                } else
                // goto
                if(cmd[0]=='g'){
                    x=0; while(x<cmdlast && cmd[x]!=' ') x++;
                    if(cmd[x]==' ') {
getgoto:                while(x<cmdlast && cmd[x]==' ') x++;
                        if(cmd[x]=='-'||cmd[x]=='+') x++;
                        y = dbg_getaddr(&cmd[x],cmdlast-x);
                        if(y==0) {
                            rip = tcb->rip;
                        } else {
                            if(cmd[x-1]=='-')
                                rip -= y;
                            else if(cmd[x-1]=='+')
                                rip += y;
                            else
                                rip = y;
                        }
                        cmdidx = cmdlast = 0;
                        cmd[cmdidx]=0;
                    } else {
                        rip = disasm(rip, NULL);
                        dbg_start = dbg_next;
                        /* don't clear the command */
                    }
                    dbg_tab = tab_code;
                    goto redraw;
                } else
                // examine
                if(cmd[0]=='x'||cmd[0]=='/'||(cmd[0]=='e'&&cmd[1]=='x')){
                    dbg_tab = tab_data;
                    x=0; while(x<cmdlast && cmd[x]!=' ' && cmd[x]!='/') x++;
                    while(x<cmdlast && cmd[x]==' ') x++;
                    if(cmd[x]=='/') {
                        x++;
                        if(cmd[x]=='1'||cmd[x]=='b')
                            dbg_unit=0;
                        else if(cmd[x]=='2'||cmd[x]=='w')
                            dbg_unit=1;
                        else if(cmd[x]=='4'||cmd[x]=='d'||cmd[x]=='l')
                            dbg_unit=2;
                        else if(cmd[x]=='8'||cmd[x]=='q')
                            dbg_unit=3;
                        else if(cmd[x]=='s')
                            dbg_unit=4;
                        else {
                            dbg_err="Unknown unit flag";
                            goto getcmd;
                        }
                        while(x<cmdlast && cmd[x]!=' ') x++;
                        while(x<cmdlast && cmd[x]==' ') x++;
                    } else if(x==cmdlast) {
                        rsp = tcb->rsp;
                    }
                    if(x<cmdlast) {
                        if(cmd[x]=='-'||cmd[x]=='+') x++;
                        y = dbg_getaddr(&cmd[x],cmdlast-x);
                        if(y==0 && cmd[x]!='0'){
                            rsp = tcb->rsp;
                            cmdidx = cmdlast = 0;
                            cmd[cmdidx]=0;
                        } else {
                            if(cmd[x-1]=='-')
                                rsp -= y;
                            else if(cmd[x-1]=='+')
                                rsp += y;
                            else
                                rsp = y;
                        }
                        /* don't clear the command */
                    } else {
                        cmdidx = cmdlast = 0;
                        cmd[cmdidx]=0;
                    }
                    goto redraw;
                } else
                // break
                if(cmd[0]=='b'){
                    x=0; while(x<cmdlast && cmd[x]!=' ' && cmd[x]!='/') x++;
                    while(x<cmdlast && cmd[x]==' ') x++;

                    m = l = DBG_MODE_EXEC;
                    if(cmd[x]=='/') {
                        while(x<cmdlast && cmd[x]!=' ') {
                            if(cmd[x]=='1'||cmd[x]=='b') {
                                l=0x00;
                            } else
                            if(cmd[x]=='2') {
                                l=0x01;
                            } else
                            if(cmd[x]=='4'||cmd[x]=='d'||cmd[x]=='l') {
                                l=0x11;
                            } else
                            if(cmd[x]=='8'||cmd[x]=='q') {
                                l=0x10;
                            } else
                            if(cmd[x]=='r') {
                                m=DBG_MODE_READ;
                                if(l==0)
                                    l=0b10;
                            } else
                            if(cmd[x]=='w') {
                                m=DBG_MODE_WRITE;
                                if(l==0)
                                    l=0b10;
                            } else
                            if(cmd[x]=='p') {
                                m=DBG_MODE_PORT;
                                if(l==0)
                                    l=0b10;
                            }
                            //nothing to do with 'x'
                            x++;
                        }
                        while(x<cmdlast && cmd[x]==' ') x++;
                    }
                    y = dbg_getaddr(&cmd[x], cmdlast-x);
                    if(y==0) {
                        fx = kx = (maxx-25)/2; ky = (maxy-6)/2; fg=dbg_theme[5]; bg=dbg_theme[2];
                        if(dbg_tui)
                            dbg_setpos();
                        __asm__ __volatile__ ("movq %%dr0, %0":"=r"(y)::);
                        kprintf("                          \n");
                        kprintf("  %cBRK0 %8x  \n",dbg_numbrk%4==0?'>':' ',y);
                        __asm__ __volatile__ ("movq %%dr1, %0":"=r"(y)::);
                        kprintf("  %cBRK1 %8x  \n",dbg_numbrk%4==1?'>':' ',y);
                        __asm__ __volatile__ ("movq %%dr2, %0":"=r"(y)::);
                        kprintf("  %cBRK2 %8x  \n",dbg_numbrk%4==2?'>':' ',y);
                        __asm__ __volatile__ ("movq %%dr3, %0":"=r"(y)::);
                        kprintf("  %cBRK3 %8x  \n",dbg_numbrk%4==3?'>':' ',y);
                        kprintf("                          \n");
                        cmdidx = cmdlast = 0;
                        cmd[cmdidx]=0;
                        goto getcmd;
                    } else
                        dbg_setbrk(y/*addr*/, m/*mode*/, l/*len*/);
                    cmdidx = cmdlast = 0;
                    cmd[cmdidx]=0;
                    dbg_tab = tab_code;
                    goto redraw;
                }
                /* unknown command, do nothing */
                dbg_err = "Unknown command";
                goto getcmd;
            }
            default: {
                if(cmdlast>=sizeof(cmd)-1) {
                    dbg_err = "Buffer full";
                    goto getcmd;
                }
                if(cmdidx<cmdlast) {
                    for(x=cmdlast;x>cmdidx;x--)
                        cmd[x]=cmd[x-1];
                }
                if(dbg_iskbd){
                    if(scancode>=2&&scancode<=13) {
                        if(dbg_isshft)
                            c = "!@#$%^&*()_+"[scancode-2];
                        else
                            c = "1234567890-="[scancode-2];
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
                } else
                    c = scancode;
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
