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
 * @brief Internal Debugger
 */

#if DEBUG

#include <sys/init.h>
#include <sys/video.h>
#include "../font.h"
#include "../../lib/libc/bztalloc.h"

// breakpoint types for setbrk
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
extern ccb_t ccb;
extern pmm_t pmm;
extern uint64_t *safestack;
extern uint8_t sys_fault;
extern uint64_t lastsym;
extern char *addr_base;
extern uint64_t bogomips;
extern uint64_t isr_lastfps;
extern char *syslog_buf;
extern char *syslog_ptr;
extern char osver[];
extern char *nosym;

extern uchar *elf_sym(virt_t addr);
extern virt_t elf_lookupsym(uchar *sym, size_t size);
extern void kprintf_putchar(int c);
extern void kprintf_unicodetable();
extern unsigned char *env_hex(unsigned char *s, uint64_t *v, uint64_t min, uint64_t max);
extern virt_t disasm(virt_t rip, char *str);
extern char *sprintf(char *dst,char* fmt, ...);
extern void sched_dump();

// variables
dataseg uint8_t dbg_enabled;
dataseg uint8_t dbg_tab;
dataseg uint8_t dbg_inst;
dataseg uint8_t dbg_full;
dataseg uint8_t dbg_unit;
dataseg uint8_t dbg_numbrk;
dataseg uint8_t dbg_iskbd;
dataseg uint8_t dbg_isshft;
dataseg uint8_t dbg_indump;
dataseg uint8_t dbg_tui;
dataseg uint64_t dbg_scr;
dataseg uint64_t dbg_mqptr;
dataseg uint64_t cr2;
dataseg uint64_t cr3;
dataseg uint64_t dr6;
dataseg uint64_t oldist1;
dataseg uint64_t dbg_logpos;
dataseg char dbg_instruction[128];
dataseg virt_t dbg_comment;
dataseg virt_t dbg_next;
dataseg virt_t dbg_start;
dataseg virt_t dbg_lastrip;
dataseg virt_t dbg_origrip;
dataseg char *dbg_err;
dataseg uint32_t *dbg_theme;
dataseg uint32_t theme_panic[] = {0x100000,0x400000,0x800000,0x9c3c1b,0xcc6c4b,0xec8c6b} ;
dataseg uint32_t theme_debug[] = {0x000020,0x000040,0x101080,0x3e32a2,0x7c71da,0x867ade} ;
//uint32_t theme_debug[] = {0x000010,0x000020,0x000040,0x1b3c9c,0x4b6ccc,0x6b8cec} ;
dataseg char *brk = "BRK? ?? set at ????????????????h";
dataseg char cmdhist[512];

// different kind of tabs
enum {
    tab_code,
    tab_data,
    tab_msg,
    tab_tcb,
    tab_queues,
    tab_ram,
    tab_sysinfo,

    tab_last
};

// priority queue names
dataseg char *prio[] = {
    "SYSTEM   ",
    "RealTime ",
    "Driver   ",
    "Service  ",
    "Important",
    "Normal   ",
    "Low prio ",
    "IDLE ONLY"
};

void dbg_putchar(int c)
{
    if(c<8 || c>255 || (dbg_indump==2 && (c<' '||c>=127)) || (c>=127&&c<160) || c==173)
        c='.';
    __asm__ __volatile__ (
        "movl %0, %%eax;movb %%al, %%bl;outb %%al, $0xe9;"
        "movl $10000,%%ecx;movw $0x3fd,%%dx;1:"
        "inb %%dx, %%al;"
        "cmpb $0xff,%%al;je 2f;"
        "dec %%ecx;jz 2f;"
        "andb $0x40,%%al;jz 1b;"
        "subb $5,%%dl;movb %%bl, %%al;outb %%al, %%dx;2:"
    ::"r"(c):"%rax","%rbx","%rdx");
}

void dbg_pagingflags(uint64_t p)
{
    kprintf("%x%c%c%c%c",(p>>9)&0x7,(p>>8)&1?'G':'.',(p>>7)&1?'T':'.',(p>>6)&1?'D':'.',(p>>5)&1?'A':'.');
    kprintf("%c%c%c%c%c",(p>>4)&1?'.':'C',(p>>3)&1?'W':'.',(p>>2)&1?'U':'S',(p>>1)&1?'W':'R',p&(1UL<<63)?'.':'X');
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

// set a breakpoint for a memory range.
// For available modes see DBG_MODE_* at top.
void dbg_setbrk(virt_t addr, uint8_t mode, uint8_t len)
{
    // set up message with highlight
    dbg_err = brk;
    brk[3]='0'+(dbg_numbrk%4);
    brk[5]=(mode==DBG_MODE_EXEC?'x':(mode==DBG_MODE_READ?'r':(mode==DBG_MODE_WRITE?'w':'p')));
    brk[6]=(len==0b00?'b':(len==0b01?'w':(len==0b11?'d':'q')));
    sprintf(brk+15,"%8x",addr);
    // set up registers
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

// dump a Task Control Block
void dbg_tcb()
{
    tcb_t *tcb = (tcb_t*)0;
    char *states[] = { "hybernated", "blocked", "running" };
    char *errn[] = { "SUCCESS", "EPERM", "ENOENT", "ESRCH", "EINTR", "EIO", "ENXIO", "E2BIG", "ENOEXEC",
        "EBADF", "ECHILD", "EAGAIN", "ENOMEM", "EACCES", "EFAULT", "ENOTBLK", "EBUSY", "EEXIST", "EXDEV",
        "ENODEV", "ENOTDIR", "EISDIR", "EINVAL", "ENFILE", "EMFILE", "ENOTTY", "ETXTBSY", "EFBIG", "ENOSPC",
        "ESPIPE", "EROFS", "EMLNK", "EPIPE", "EDOM", "ERANGE" };
    uint64_t t;
    int mask[]={1,2,4,8,16,32,64,128,256,512,1024,2048,4096,8192,16384,32768};
    char *bmap=".123456789ABCDEF";
    uint16_t *m;
    uint64_t *arena=(uint64_t*)BSS_ADDRESS;
    uint64_t i,j,k,l,n,o;

    if(!dbg_full) {
        fg=dbg_theme[4];
        if(dbg_tui)
            dbg_settheme();
        kprintf("[Task Control Block]\n");
        fg=dbg_theme[3];
        if(dbg_tui)
            dbg_settheme();
        kprintf("pid: %4x, name: %s\nRunning on cpu core: %x, ",tcb->pid, tcb->name, tcb->cpu);
        kprintf("priority: %s (%d), state: %s (%1x) ",
            prio[tcb->priority], tcb->priority,
            tcb->state<sizeof(states)? states[tcb->state] : "???", tcb->state
        );
        kprintf("\nerrno: %d %s, exception error code: %d\n",
            tcb->errno, tcb->errno<sizeof(errn)? errn[tcb->errno] : "???", tcb->excerr);
    
        fg=dbg_theme[4];
        if(dbg_tui)
            dbg_settheme();
        kprintf("\n[Scheduler]\n");
        fg=dbg_theme[3];
        if(dbg_tui)
            dbg_settheme();
        t=(tcb->state==tcb_state_running?0:ticks[TICKS_LO]-tcb->blktime) + tcb->blkcnt;
        kprintf("user ticks: %9d, blocked ticks: %9d, blocked since: %9d\n", tcb->billcnt, t, tcb->blktime);
        kprintf("next in alarm queue: %4x, alarm at: %d.%d\n", tcb->alarm, tcb->alarmsec, tcb->alarmns);
        kprintf("next task in queue: %4x, previous task in queue: %4x\n", tcb->next, tcb->prev);
        kprintf("\n");
    }
    if(tcb->state==tcb_state_hybernated) {
        fg=dbg_theme[4];
        if(dbg_tui)
            dbg_settheme();
        kprintf("[Hybernation info]\n");
        fg=dbg_theme[3];
        if(dbg_tui)
            dbg_settheme();
        kprintf("swapped to: %8x\n", tcb->swapminor);
    } else {
        fg=dbg_theme[4];
        if(dbg_tui)
            dbg_settheme();
        kprintf("[Memory]\n");
        fg=dbg_theme[3];
        if(dbg_tui)
            dbg_settheme();
        kprintf("mapping: %8x ", tcb->memroot); dbg_pagingflags(tcb->memroot);
        kprintf(", allocated pages: %6d, linked pages: %6d\n", tcb->allocmem, tcb->linkmem);
    
        fg=dbg_theme[4];
        if(dbg_tui)
            dbg_settheme();
        kprintf("\n[Virtual Memory Manager%s, allocmaps: %d/%d, chunks: %d, %d units]\n",
            (arena[0] & (1UL<<63))!=0?" (LOCKED)":"",numallocmaps(arena), MAXARENA,
            (ALLOCSIZE-8)/sizeof(chunkmap_t), BITMAPSIZE*64);
        kprintf("Idx Quantum Start       End       Allocation bitmap                    Size\n");
        fg=dbg_theme[3];
        if(dbg_tui)
            dbg_settheme();
    
        o=1;
        for(i=1;i<MAXARENA && arena[i]!=0;i++) {
            if(i>128) break;
            if(((allocmap_t*)arena[i])->numchunks==0)
                continue;
            fg=dbg_theme[2];
            if(dbg_tui)
                dbg_settheme();
            kprintf("  --- allocmap %d @%x, numchunks: %d ---\n",i,arena[i],((allocmap_t*)arena[i])->numchunks);
            fg=dbg_theme[3];
            if(dbg_tui)
                dbg_settheme();
            for(j=0;j<((allocmap_t*)arena[i])->numchunks;j++) {
                if(j>128 || (((allocmap_t*)arena[i])->chunk[j].quantum&7)!=0) break;
                kprintf("%3d. %9d %x - %x ",o++,
                    ((allocmap_t*)arena[i])->chunk[j].quantum,
                    ((allocmap_t*)arena[i])->chunk[j].ptr,
                    ((allocmap_t*)arena[i])->chunk[j].ptr+
                        (((allocmap_t*)arena[i])->chunk[j].quantum<ALLOCSIZE?
                            chunksize(((allocmap_t*)arena[i])->chunk[j].quantum):
                            ((allocmap_t*)arena[i])->chunk[j].map[0]));
                if(((allocmap_t*)arena[i])->chunk[j].quantum<ALLOCSIZE) {
                    m=(uint16_t*)&((allocmap_t*)arena[i])->chunk[j].map;
                    for(k=0;k<BITMAPSIZE*4;k++) {
                        l=0; for(n=0;n<16;n++) { if(*m & mask[n]) l++; }
                        kprintf("%c",bmap[l]);
                        m++;
                    }
                    kprintf("%8dk\n",chunksize(((allocmap_t*)arena[i])->chunk[j].quantum)/1024);
                } else {
                    kprintf("(no bitmap) %8dM\n",((allocmap_t*)arena[i])->chunk[j].map[0]/1024/1024);
                }
            }
        }
    }
}

uint64_t dbg_getrip(int *idx)
{
    tcb_t *tcb = (tcb_t*)0;
    uint64_t i, *rsp=(uint64_t*)(tcb->rsp), *o;
    int j=0;
    uchar *symstr;
    o=rsp;
    if(j==*idx) return dbg_origrip;
    sys_fault = false;
    i=0;
    if(*idx<0) *idx=0;
    while(i++<4 && !sys_fault && rsp!=0 && 
        ((uint64_t)rsp<(uint64_t)TEXT_ADDRESS||(uint64_t)rsp>(uint64_t)&__bss_start)){
        if((((rsp[1]==0x23||rsp[1]==0x08) &&
            (rsp[4]==0x1b||rsp[4]==0x18))) && !sys_fault){
            if(j==*idx)
                return *rsp;
            symstr=elf_sym(*rsp);
            j++;
            rsp=(uint64_t*)rsp[3];
            o=0;
            continue;
        }
        if(sys_fault)
            break;
        if((*rsp>TEXT_ADDRESS && *rsp<BSS_ADDRESS) ||
           (*rsp>((uint64_t)&__bss_start))) {
            if(sys_fault)
                break;
            symstr=elf_sym(*rsp);
            if(sys_fault)
                break;
            if((virt_t)symstr>(virt_t)TEXT_ADDRESS &&
                (virt_t)symstr<(virt_t)BSS_ADDRESS && kstrcmp(symstr,"_edata")) {
                if(j==*idx)
                    return *rsp;
                j++;
            }
        }
        rsp++;
    }
    if(o) {
        rsp=o; i=0;
        while(i++<4 && !sys_fault && rsp!=0 && ((uint64_t)rsp<(uint64_t)TEXT_ADDRESS||(uint64_t)rsp>(uint64_t)&__bss_start)){
            if((*rsp>TEXT_ADDRESS && *rsp<BSS_ADDRESS) ||
               (*rsp>((uint64_t)&__bss_start))) {
                if(sys_fault)
                    break;
                symstr=elf_sym(*rsp);
                if(sys_fault)
                    break;
                if((virt_t)symstr>(virt_t)TEXT_ADDRESS &&
                    (virt_t)symstr<(virt_t)BSS_ADDRESS && kstrcmp(symstr,"_edata")) {
                    if(j==*idx)
                        return *rsp;
                    j++;
                }
            }
            rsp++;
        }
    }
    j--; if(*idx>j) *idx=j;
    return dbg_origrip;
}

// disassembly instructions and dump registers, stack
void dbg_code(uint64_t rip, uint64_t rs, int *idx)
{
    tcb_t *tcb = (tcb_t*)0;
    uchar *symstr;
    virt_t ptr, dmp, lastsymsave;
    int i=0;
    uint64_t j, *rsp=(uint64_t*)(rs!=0?rs:tcb->rsp), *o;

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
        if(dbg_inst) {
            kprintf("rax=%8x rbx=%8x \n",*((uint64_t*)(tcb->gpr+0)),  *((uint64_t*)(tcb->gpr+8)));
            kprintf("rcx=%8x rdx=%8x \n",*((uint64_t*)(tcb->gpr+16)), *((uint64_t*)(tcb->gpr+24)));
            kprintf("rsi=%8x rdi=%8x \n",*((uint64_t*)(tcb->gpr+32)), *((uint64_t*)(tcb->gpr+40)));
            kprintf(" r8=%8x  r9=%8x \n",*((uint64_t*)(tcb->gpr+48)), *((uint64_t*)(tcb->gpr+56)));
            kprintf("r10=%8x r11=%8x \n",*((uint64_t*)(tcb->gpr+64)), *((uint64_t*)(tcb->gpr+72)));
            kprintf("r12=%8x r13=%8x \n",*((uint64_t*)(tcb->gpr+80)), *((uint64_t*)(tcb->gpr+88)));
            kprintf("r14=%8x r15=%8x \n",*((uint64_t*)(tcb->gpr+96)), *((uint64_t*)(tcb->gpr+104)));
        } else {
            kprintf("rax='%A' rbx='%A' \n",*((uint64_t*)(tcb->gpr+0)),  *((uint64_t*)(tcb->gpr+8)));
            kprintf("rcx='%A' rdx='%A' \n",*((uint64_t*)(tcb->gpr+16)), *((uint64_t*)(tcb->gpr+24)));
            kprintf("rsi='%A' rdi='%A' \n",*((uint64_t*)(tcb->gpr+32)), *((uint64_t*)(tcb->gpr+40)));
            kprintf(" r8='%A'  r9='%A' \n",*((uint64_t*)(tcb->gpr+48)), *((uint64_t*)(tcb->gpr+56)));
            kprintf("r10='%A' r11='%A' \n",*((uint64_t*)(tcb->gpr+64)), *((uint64_t*)(tcb->gpr+72)));
            kprintf("r12='%A' r13='%A' \n",*((uint64_t*)(tcb->gpr+80)), *((uint64_t*)(tcb->gpr+88)));
            kprintf("r14='%A' r15='%A' \n",*((uint64_t*)(tcb->gpr+96)), *((uint64_t*)(tcb->gpr+104)));
        }
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
        o=rsp;
        while(i++<11 && !sys_fault && ((uint64_t)rsp<(uint64_t)TEXT_ADDRESS||(uint64_t)rsp>(uint64_t)&__bss_start)){
            kprintf("rsp+%1x rbp-%1x: ",
                !dbg_inst?((uint64_t)rsp)&0xFF:(uint32_t)(uint64_t)rsp - (uint64_t)tcb->rsp,
                (uint32_t)((uint64_t)(tcb->gpr+112)-(uint64_t)rsp)
            );
            kprintf(dbg_inst?"%8x \n":"'%A' \n", *rsp);
            rsp++;
        }
        if(dbg_tui) {
            dbg_setpos();
        }
        fg = dbg_theme[2];
        if(dbg_tui)
            dbg_settheme();
        kprintf(sys_fault?"  * not mapped *\n":"  * end *\n");
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
        if(*idx==0)
            fg=0xFFDD33;
        else
            fg=dbg_theme[3];
        if(dbg_tui)
            dbg_settheme();
        kprintf("%8x: %s \n",
            dbg_origrip, elf_sym(dbg_origrip)
        );
        fg=dbg_theme[3];
        if(dbg_tui)
            dbg_settheme();
        sys_fault = false;
        i=0;j=1;
        while(i++<4 && !sys_fault && rsp!=0 && ((uint64_t)rsp<(uint64_t)TEXT_ADDRESS||(uint64_t)rsp>(uint64_t)&__bss_start)){
            if((((rsp[1]==0x23||rsp[1]==0x08) &&
                (rsp[4]==0x1b||rsp[4]==0x18))) && !sys_fault) {
                if(*idx==j)
                    fg=0xFFDD33;
                else
                    fg=dbg_theme[3];
                if(dbg_tui)
                    dbg_settheme();
                kprintf("%8x: %s   * interrupt %x * \n",*rsp, elf_sym(*rsp), rsp[3]);
                j++;
                fg=dbg_theme[3];
                if(dbg_tui)
                    dbg_settheme();
                rsp=(uint64_t*)rsp[3];
                o=0;
                continue;
            }
            if(sys_fault)
                break;
            if((*rsp>TEXT_ADDRESS && *rsp<BSS_ADDRESS) ||
               (*rsp>((uint64_t)&__bss_start))) {
                if(sys_fault)
                    break;
                symstr = elf_sym(*rsp);
                if(sys_fault)
                    break;
                if((virt_t)symstr>(virt_t)TEXT_ADDRESS &&
                    (virt_t)symstr<(virt_t)BSS_ADDRESS && kstrcmp(symstr,"_edata")) {
                    if(*idx==j)
                        fg=0xFFDD33;
                    else
                        fg=dbg_theme[3];
                    if(dbg_tui)
                        dbg_settheme();
                    kprintf("%8x: %s \n", *rsp, symstr);
                    j++;
                    fg=dbg_theme[3];
                    if(dbg_tui)
                        dbg_settheme();
                }
            }
            rsp++;
        }
        if(o) {
            rsp=o; i=0;
            while(i++<4 && !sys_fault && rsp!=0 && ((uint64_t)rsp<(uint64_t)TEXT_ADDRESS||(uint64_t)rsp>(uint64_t)&__bss_start)){
                if((*rsp>TEXT_ADDRESS && *rsp<BSS_ADDRESS) ||
                   (*rsp>((uint64_t)&__bss_start))) {
                    if(sys_fault)
                        break;
                    symstr = elf_sym(*rsp);
                    if(sys_fault)
                        break;
                    if((virt_t)symstr>(virt_t)TEXT_ADDRESS &&
                        (virt_t)symstr<(virt_t)BSS_ADDRESS && kstrcmp(symstr,"_edata")) {
                        if(*idx==j)
                            fg=0xFFDD33;
                        else
                            fg=dbg_theme[3];
                        if(dbg_tui)
                            dbg_settheme();
                        kprintf("%8x: %s \n", *rsp, symstr);
                        j++;
                        fg=dbg_theme[3];
                        if(dbg_tui)
                            dbg_settheme();
                    }
                }
                rsp++;
            }
        }
        kprintf("\n");
    }

    /* disassemble block */
    kx=fx=2;
    fg=dbg_theme[4];
    if(dbg_tui)
        dbg_settheme();
    sys_fault = false;
    symstr = elf_sym(rip);
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
    /* find a start position. This is tricky on x86 */
    if(/*dbg_start == rip ||*/ dbg_start == 0) {
        dbg_next = dbg_start = rip;
        do {
            dbg_start--;
        } while(dbg_start>dbg_next-15 && disasm(dbg_start,NULL) != dbg_next);
    }
/*
    j = dbg_start && dbg_start > rip-63 && dbg_start <= rip ? dbg_start :
        (dbg_lastrip && dbg_lastrip > rip-63 && dbg_lastrip <= rip ? dbg_lastrip :
        rip-15);
*/
    j = dbg_start;
    if(lastsym && j < lastsym)
        j = lastsym;
again:
    i = 0;
    dbg_start = j;
    ptr = j;
    while(!i && ptr<=rip) {
        sys_fault = false;
        while(!sys_fault) {
            if(ptr == rip) {
                i = 1;
                break;
            }
            ptr = disasm((virt_t)ptr, NULL);
        }
    }
    if(!i && j>rip-63) { j--; goto again; }
    dbg_next = disasm(dbg_start, NULL);
    ptr = dbg_start;
    while(ky < maxy-2) {
        fg= ptr==rip?0xFFDD33:dbg_theme[3];
        if(dbg_tui)
            dbg_settheme();
        kprintf("%2x %c%c ",!dbg_inst?((uint64_t)ptr)&0xffffffffffff:(uint64_t)ptr-lastsym,ptr==rip||ptr==dbg_lastrip?
            ((dbg_lastrip<rip?dbg_lastrip:rip)==ptr||dbg_lastrip==0?(dbg_lastrip==rip||dbg_lastrip==0?'-':'/'):'\\')
            :(
            dbg_lastrip!=0&&((dbg_lastrip<rip && ptr>dbg_lastrip && ptr<rip)||
            (dbg_lastrip>rip && ptr>rip && ptr<dbg_lastrip))?'|':' '),
            ptr==rip?'>':(dbg_lastrip!=0&&ptr==dbg_lastrip?'-':' '));
        dbg_comment = 0;
        dmp = ptr;
        lastsymsave = lastsym;
        sys_fault = false;
        ptr = disasm((virt_t)ptr, dbg_instruction);
        if(sys_fault) {
            fg = dbg_theme[2];
            if(dbg_tui)
                dbg_settheme();
            kprintf(" * not mapped *");
            fg = dbg_theme[3];
            if(dbg_tui)
                dbg_settheme();
            return;
        }
        if(dbg_inst) {
            if(*((uint8_t*)ptr-1) == 0x90) {
                fg = dbg_theme[2];
                if(dbg_tui)
                    dbg_settheme();
            }
            kprintf("%s", dbg_instruction);
        } else {
            for(;dmp<ptr;dmp++)
                kprintf("%1x ",*((uchar*)dmp));
        }
        if(dbg_comment) {
            fg=dbg_theme[2];
            kx=maxx/2+10;
            sys_fault = false; symstr=NULL;
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
                if(dbg_comment>TEXT_ADDRESS && dbg_comment<BSS_ADDRESS) {
                    symstr = elf_sym(dbg_comment);
                }
            }
            if(!sys_fault && symstr!=NULL && symstr!=(uchar*)nosym) {
                if(dbg_tui)
                    dbg_setpos();
                else {
                    dbg_putchar(9);
                    dbg_putchar(9);
                    dbg_putchar(';');
                }
                if(symstr != (uchar*)&dbg_comment)
                    kprintf("%s +%x", symstr, (uint32_t)(dbg_comment-lastsym));
                else {
                    kprintf("'%s'", symstr);
                }
            }
        }
        lastsym = lastsymsave;
        kprintf("\n");
    }
}

// dump memory in bytes, words, double words or quad words
void dbg_data(uint64_t ptr)
{
    tcb_t *tcb = (tcb_t*)0;
    int i,j;
    uint64_t o=ptr,d;
    uchar *symstr;
    phy_t tmp3 = (*((uint64_t*)kmap_getpte((virt_t)&tmp3map)))&~(__PAGESIZE-1+(1UL<<63));

    ky = 3;
    uint64_t *paging = (uint64_t *)&tmp2map;
    if(dbg_tui)
        dbg_setpos();
    while(ky<maxy-(dbg_full?2:5)) {
        kx=1;
        if(ptr>=(uint64_t)&tmp3map && ptr<(uint64_t)&tmp3map+__PAGESIZE)
            kprintf("phy %6x: ", tmp3+ptr-(uint64_t)&tmp3map);
        else
            kprintf("%8x: ",ptr);
        sys_fault = false;
        __asm__ __volatile__ ("movq %0, %%rdi;movb (%%rdi), %%al"::"m"(ptr):"%rax","%rdi");
        if(sys_fault) {
            fg = dbg_theme[2];
            if(dbg_tui)
                dbg_settheme();
            kprintf(" * not mapped *");
            fg = dbg_theme[3];
            if(dbg_tui)
                dbg_settheme();
        }
        dbg_indump = 2;
        switch(dbg_unit) {
            // stack view
            case 4:
                if(!sys_fault) {
                    kx = 19; kprintf("%8x ",*((uint64_t*)ptr));
                    kx = 36;
                    kprintf_putchar(*((uint8_t*)ptr)); kx++;
                    kprintf_putchar(*((uint8_t*)(ptr+1))); kx++;
                    kprintf_putchar(*((uint8_t*)(ptr+2))); kx++;
                    kprintf_putchar(*((uint8_t*)(ptr+3))); kx++;
                    kprintf_putchar(*((uint8_t*)(ptr+4))); kx++;
                    kprintf_putchar(*((uint8_t*)(ptr+5))); kx++;
                    kprintf_putchar(*((uint8_t*)(ptr+6))); kx++;
                    kprintf_putchar(*((uint8_t*)(ptr+7)));
                    if((ptr>__PAGESIZE && ptr<=TEXT_ADDRESS) || ptr>CORE_ADDRESS) {
                        if(ptr==TEXT_ADDRESS)
                            symstr=(unsigned char*)"  ^^^ STACK ^^^";
                        else
                            symstr = elf_sym(*((uint64_t*)ptr));
                        if(!sys_fault && symstr!=NULL && symstr!=(uchar*)nosym) {
                            dbg_indump = false;
                            fg = dbg_theme[2];
                            if(dbg_tui)
                                dbg_settheme();
                            kx = 44;
                            kprintf("  %s",symstr);
                            fg = dbg_theme[3];
                            if(dbg_tui)
                                dbg_settheme();
                            dbg_indump = 2;
                        }
                    }
                    sys_fault = false;
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
        dbg_indump = true;
        kprintf("\n");
    }
    if(!dbg_full) {
        kmap((uint64_t)&tmp2map, tcb->memroot, PG_CORE_NOCACHE);
        d=(o>>(12+9+9+9))&0x1FF;
        if((paging[d]&1)==0) {fg = dbg_theme[2];if(dbg_tui) dbg_settheme();else dbg_putchar('!');}
        kprintf("\n  PML4 %8x ",paging[d]);
        dbg_pagingflags(paging[d]);
        if(paging[d]&1) {
            kmap((uint64_t)&tmp2map, paging[d], PG_CORE_NOCACHE);
            d=(o>>(12+9+9))&0x1FF;
            if((paging[d]&1)==0) {fg = dbg_theme[2];if(dbg_tui) dbg_settheme();else dbg_putchar('!');}
            kprintf(" %4d      PDPE %8x ",d,paging[d]);
            dbg_pagingflags(paging[d]);
            if(paging[d]&1){
                kmap((uint64_t)&tmp2map, paging[d], PG_CORE_NOCACHE);
                d=(o>>(12+9))&0x1FF;
                if((paging[d]&1)==0) {fg = dbg_theme[2];if(dbg_tui) dbg_settheme();else dbg_putchar('!');}
                kprintf(" %4d\n  PDE  %8x ",d,paging[d]);
                dbg_pagingflags(paging[d]);
                if(paging[d]&1){
                    if(paging[d]&PG_SLOT) {
                        d=(o>>(12))&0x1FF;
                        kprintf(" %4d      (slot allocated)\n",d);
                    } else {
                        kmap((uint64_t)&tmp2map, paging[d], PG_CORE_NOCACHE);
                        d=(o>>(12))&0x1FF;
                        if((paging[d]&1)==0) {fg = dbg_theme[2];if(dbg_tui) dbg_settheme();else dbg_putchar('!');}
                        kprintf(" %4d      PTE  %8x ",d,paging[d]);
                        dbg_pagingflags(paging[d]);
                        d=o&(__PAGESIZE-1);
                        kprintf(" %4d\n",d);
                    }
                }
            }
        }
    }
}

// list messages in Task Message Queue
void dbg_msg()
{
    tcb_t *tcb = (tcb_t*)0;
    uint64_t m, i, t, args;

    fg=dbg_theme[4];
    if(dbg_tui)
        dbg_settheme();
    if(!dbg_full) {
        kprintf("[Queue Header]\n");
        fg=dbg_theme[3];
        if(dbg_tui)
            dbg_settheme();
        kprintf("msg in: %3d, msg out: %3d, msg max: %3d, recv from: %x\n",
            *((uint64_t*)(MQ_ADDRESS)), *((uint64_t*)(MQ_ADDRESS+8)),
            *((uint64_t*)(MQ_ADDRESS+16)),*((uint64_t*)(MQ_ADDRESS+24)));
        kprintf("buf in: %3d, buf out: %3d, buf max: %3d, buf min: %3d, out serial: %d\n",
            *((uint64_t*)(MQ_ADDRESS+32)), *((uint64_t*)(MQ_ADDRESS+40)),
            *((uint64_t*)(MQ_ADDRESS+48)),*((uint64_t*)(MQ_ADDRESS+56)),tcb->serial+1);
        fg=dbg_theme[4];
        if(dbg_tui)
            dbg_settheme();
        kprintf("\n");
    }
    kprintf("[Messages]");
    /* print out messages */
    i = dbg_mqptr;
    do {
        t = i==*((uint64_t*)MQ_ADDRESS)?4:(
            *((uint64_t*)MQ_ADDRESS)!=*((uint64_t*)MQ_ADDRESS+8) &&
                (i>=*((uint64_t*)MQ_ADDRESS)||i<*((uint64_t*)MQ_ADDRESS+8))?2:3);
        fg = t==4 ? 0xFFDD33 : dbg_theme[t];
        if(dbg_tui)
            dbg_settheme();
        m = ((i * sizeof(msg_t))+(uint64_t)MQ_ADDRESS);
        kprintf("\n %2x: ", m);
        if(*((uint64_t*)m)==0 && *((uint64_t*)m+8)==0 && *((uint64_t*)m+16)==0) {
            fg = dbg_theme[0];
            if(dbg_tui)
                dbg_settheme();
            kprintf("* empty *\n");
        } else {
            args=1;
            kprintf("from %x ", *((uint64_t*)m) >>16);
            if(*((uint8_t*)m)<3) {
                switch(*((uint8_t*)m)) {
                    case SYS_IRQ: kprintf("IRQ(%d)",*((uint64_t*)(m+8))); args=0; break;
                    case SYS_ack: kprintf("ack(%d)",*((uint64_t*)(m+8))); args=0; break;
                    case SYS_nack: kprintf("nack(%d)",*((uint64_t*)(m+16))); args=0; break;
                }
            } else {
                if(tcb->pid == services[-SRV_FS]) {
                    switch(*((uint8_t*)m)) {
                        case SYS_read: kprintf("read"); break;
                        case SYS_dup2: kprintf("dup2"); break;
                        case SYS_pipe: kprintf("pipe"); break;
                        case SYS_write: kprintf("write"); break;
                        case SYS_seek: kprintf("seek"); break;
                        case SYS_dup: kprintf("dup"); break;
                        case SYS_stat: kprintf("stat"); break;
                        case SYS_ioctl: kprintf("ioctl"); break;
                        case SYS_mountfs: kprintf("mountfs()"); args=0; break;
                        default: kprintf("%2x?", *((uint64_t*)m) &0x7FFF); break;
                    }
                } else
                if(tcb->pid == services[-SRV_UI]) {
                    switch(*((uint8_t*)m)) {
                        case SYS_keypress: kprintf("keypress(%d, '%a')",*((uint64_t*)(m+8)),*((uint64_t*)(m+16))); args=0; break;
                        case SYS_keyrelease: kprintf("keyrelease(%d, '%a')",*((uint64_t*)(m+8)),*((uint64_t*)(m+16))); args=0; break;
                        case SYS_setcursor: kprintf("setcursor(%d, %d)",*((uint64_t*)(m+8)),*((uint64_t*)(m+16))); args=0; break;
                        case SYS_pointer: kprintf("pointer(%x, %d, %d, %d, %d)",*((uint64_t*)(m+8)),*((uint64_t*)(m+16)),
                            *((uint64_t*)(m+24)),*((uint64_t*)(m+32)),*((uint64_t*)(m+40))); args=0; break;
                        default: kprintf("%2x?", *((uint64_t*)m) &0x7FFF); break;
                    }
                } else
                if(tcb->pid == services[-SRV_syslog]) {
                    switch(*((uint8_t*)m)) {
                        case SYS_vsyslog: kprintf("vsyslog"); break;
                        case SYS_openlog: kprintf("openlog"); break;
                        case SYS_closelog: kprintf("closelog"); break;
                        case SYS_syslog: kprintf("syslog"); break;
                        default: kprintf("%2x?", *((uint64_t*)m) &0x7FFF); break;
                    }
                } else
                if(tcb->pid == services[-SRV_net]) {
                    switch(*((uint8_t*)m)) {
                        case SYS_recvfrom: kprintf("recvfrom"); break;
                        case SYS_connect: kprintf("connect"); break;
                        case SYS_getsockopt: kprintf("getsockopt"); break;
                        case SYS_bind: kprintf("bind"); break;
                        case SYS_socket: kprintf("socket"); break;
                        case SYS_getpeername: kprintf("getpeername"); break;
                        case SYS_setsockopt: kprintf("setsockopt"); break;
                        case SYS_sendto: kprintf("sendto"); break;
                        case SYS_getsockname: kprintf("getsockname"); break;
                        case SYS_sendmsg: kprintf("sendmsg"); break;
                        case SYS_listen: kprintf("listen"); break;
                        case SYS_accept: kprintf("accept"); break;
                        case SYS_shutdown: kprintf("shutdown"); break;
                        case SYS_socketpair: kprintf("socketpair"); break;
                        case SYS_recvmsg: kprintf("recvmsg"); break;
                       default: kprintf("%2x?", *((uint64_t*)m) &0x7FFF); break;
                    }
                } else
                if(tcb->pid == services[-SRV_sound]) {
                    switch(*((uint8_t*)m)) {
                        default: kprintf("%2x?", *((uint64_t*)m) &0x7FFF); break;
                    }
                } else
                if(tcb->pid == services[-SRV_init]) {
                    switch(*((uint8_t*)m)) {
                        case SYS_stop: kprintf("stop"); break;
                        case SYS_start: kprintf("start"); break;
                        case SYS_restart: kprintf("restart"); break;
                        case SYS_status: kprintf("status"); break;
                        default: kprintf("%2x?", *((uint64_t*)m) &0x7FFF); break;
                    }
                } else
                if(tcb->pid == services[-SRV_video]) {
                    switch(*((uint8_t*)m)) {
                        case VID_flush: kprintf("flush()"); args=0; break;
                        case VID_loadcursor: kprintf("loadcursor(%x)",*((uint64_t*)(m+8))); args=0; break;
                        case VID_movecursor: kprintf("movecursor(%d, %d, %d)",*((uint64_t*)(m+8)),
                            *((uint64_t*)(m+16)),*((uint64_t*)(m+24))); args=0; break;
                        case VID_setcursor: kprintf("setcursor(%d, %d)",*((uint64_t*)(m+8)),*((uint64_t*)(m+16))); args=0; break;
                        default: kprintf("%2x?", *((uint64_t*)m) &0x7FFF); break;
                    }
                } else
                if(tcb->priority==PRI_DRV) {
                    switch(*((uint8_t*)m)) {
                        default: kprintf("%2x?", *((uint64_t*)m) &0x7FFF); break;
                    }
                } else
                    kprintf("%2x", *((uint64_t*)m) &0x7FFF);
                if(args)
                    kprintf("(%x, %x, %x, %x, %x, %x)",
                        *((uint64_t*)(m+8)), *((uint64_t*)(m+16)), *((uint64_t*)(m+24)),
                        *((uint64_t*)(m+32)),*((uint64_t*)(m+40)), *((uint64_t*)(m+48))
                        );
            }
            kprintf("\n %s  serial %9d\n",
                t==4?"next":(t==3?"pend":"done"),
                *((uint64_t*)(m+56)));
        }
        i++;
        if(i>=*((uint64_t*)(MQ_ADDRESS+16))-1)
            i=1;
    } while (ky<maxy-3);
}

// dump CPU Control Block and priority queues
void dbg_queues()
{
    tcb_t *tcbq = (tcb_t*)&tmp3map;
    int i, j;
    pid_t n;

    fg=dbg_theme[4];
    if(dbg_tui)
        dbg_settheme();
    kprintf("[CPU Control Block]\n");
    fg=dbg_theme[3];
    if(dbg_tui)
        dbg_settheme();
    kprintf("Cpu real id: %d, logical id: %d, ",ccb.realid, ccb.id);
    kprintf("last xreg: %4x, mutex: %4x%4x%4x\n", ccb.lastxreg, ccb.mutex[0], ccb.mutex[1], ccb.mutex[2]);

    if(dbg_full) {
        fg=dbg_theme[4];
        if(dbg_tui)
            dbg_settheme();
        kprintf("\n[Task Queues]\n");
        fg=dbg_theme[3];
        if(dbg_tui)
            dbg_settheme();
        sched_dump();
    } else {
        kprintf("ist1: %x, ist2: %x, ist3: %x\n", oldist1, ccb.ist2, ccb.ist3);
        fg=dbg_theme[4];
        if(dbg_tui)
            dbg_settheme();
        kprintf("\n[Active Priority Queues]\n Queue number Head      Current   #Tasks\n");
        fg=dbg_theme[3];
        if(dbg_tui)
            dbg_settheme();
        for(i=0;i<8;i++) {
            j=0;
            if(ccb.hd_active[i]) {
                n = ccb.hd_active[i];
                do {
                    j++;
                    kmap((virt_t)&tmp3map, n * __PAGESIZE, PG_CORE_NOCACHE);
                    // failsafe
                    if( n == tcbq->next)
                        break;
                    n = tcbq->next;
                } while(n!=0 && j<65536);
            }
            kprintf(" %d. %s %4x, %4x, %6d\n", i, prio[i], ccb.hd_active[i], ccb.cr_active[i],j);
        }
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
                kmap((virt_t)&tmp3map, n * __PAGESIZE, PG_CORE_NOCACHE);
                // failsafe
                if( n == tcbq->next)
                    break;
                n = tcbq->next;
            } while(n!=0 && j<65536);
        }
        kprintf("iowait head: %4x, %d task(s)\n",
            ccb.hd_blocked, j);
        kprintf("timerq head: %4x, awake at %d.%d\n",
            ccb.hd_timerq,
            ccb.hd_timerq?((tcb_t*)&tmpalarm)->alarmsec:0, 
            ccb.hd_timerq?((tcb_t*)&tmpalarm)->alarmns:0);
    }
}

// show info on Physical Memory
void dbg_ram()
{
    pmm_entry_t *fmem = pmm.entries;
    int i = pmm.size;
 
    fg=dbg_theme[4];
    if(dbg_tui)
        dbg_settheme();
    if(!dbg_full) {
        kprintf("[Physical Memory Manager]\n");
        fg=dbg_theme[3];
        if(dbg_tui)
            dbg_settheme();
        kprintf("Core bss: %8x - %8x, %d pages\n\n",
            pmm.bss, pmm.bss_end,(pmm.bss_end-pmm.bss)/__PAGESIZE
        );
        kprintf("Total: %9d pages, allocated: %9d pages, free: %9d pages\n",
            pmm.totalpages, pmm.totalpages-pmm.freepages, pmm.freepages
        );
        if(!dbg_tui)
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
            kprintf_putchar('_'); kx++;
        }
        fg = dbg_theme[3];
        bg = dbg_theme[1];
        if(dbg_tui)
            dbg_settheme();
        else
            dbg_putchar(']');
        kprintf(" %2d.%d%%\n\n",
            (pmm.totalpages-pmm.freepages)*100/(pmm.totalpages+1), ((pmm.totalpages-pmm.freepages)*1000/(pmm.totalpages+1))%10
        );
        fg=dbg_theme[4];
        if(dbg_tui)
            dbg_settheme();
    }
    kprintf("[Free Memory Fragments #%d]\nAddress           Num pages\n",pmm.size);
    fg=dbg_theme[3];
    if(dbg_tui)
        dbg_settheme();
    fmem += dbg_scr;
    for(i=dbg_scr;i<pmm.size&&ky<maxx-2;i++) {
        kprintf("%8x, %9d\n",fmem->base, fmem->size);
        fmem++;
    }
}

// dump System Info
void dbg_sysinfo()
{
    char *disp[] = { "mono color", "stereo anaglyph", "stereo color" };
    char *fbt[] = { "ARGB", "RGBA", "ABGR", "BGRA" };
    if(!dbg_full) {
        fg=dbg_theme[4];
        if(dbg_tui)
            dbg_settheme();
        kprintf("[Operating System]\n");
        fg=dbg_theme[3];
        if(dbg_tui)
            dbg_settheme();
        kprintf("%s",&osver);
    
        fg=dbg_theme[4];
        if(dbg_tui)
            dbg_settheme();
        kprintf("[System Information]\n");
        fg=dbg_theme[3];
        if(dbg_tui)
            dbg_settheme();
        kprintf("time: %d.%d, overall ticks: %d *2^64 + %d\n",
            ticks[TICKS_TS], ticks[TICKS_NTS], ticks[TICKS_HI], ticks[TICKS_LO]);
        kprintf("seed: %8x%8x%8x%8x\n", srand[0], srand[1], srand[2], srand[3]);
    
        kprintf("cpu: %d bogomips, quantum: %d ticks\nnrphymax: %d, nrmqmax: %d, nrlogmax: %d, ",
            bogomips, quantum, nrphymax, nrmqmax, nrlogmax);
        kprintf("keyboard map: %a, debug flags: %x\nidentity: %s, rescueshell: %s, lefthanded: %s, ",
            keymap, debug, identity?"true":"false", rescueshell?"true":"false", lefthanded?"true":"false");
        kprintf("display: %d %s\n",
            display, disp[display]);
        kprintf("framebuffer: @%8x, %dx%d, scanline %d,",
            bootboot.fb_ptr, bootboot.fb_width, bootboot.fb_height, bootboot.fb_scanline);
        kprintf(" type: %d %s, fps: %d\n\n",bootboot.fb_type,fbt[bootboot.fb_type], isr_lastfps);
    
        fg=dbg_theme[4];
        if(dbg_tui)
            dbg_settheme();
        kprintf("[System Tables]\n");
        fg=dbg_theme[3];
        if(dbg_tui)
            dbg_settheme();
        kprintf("   acpi: %8x,   smbi: %8x,   uefi: %8x\n     mp: %8x,",
            systables[0], systables[1], systables[2], systables[3]);
        kprintf("   apic: %8x,   dsdt: %8x\n",
            systables[4], systables[5]);
        kprintf(" ioapic: %8x,   hpet: %8x\n\n",
            systables[7], systables[8]);
    
        fg=dbg_theme[4];
        if(dbg_tui)
            dbg_settheme();
        kprintf("[Syslog buffer]\n");
        fg=dbg_theme[3];
        if(dbg_tui)
            dbg_settheme();
    } else
        ky=2;
    kprintf("\r");
    fx=kx=0; maxy-=2;
    kprintf("%s",syslog_buf+dbg_logpos);
    maxy+=2;
    kprintf("\n");
}

// switch to the previous task
void dbg_switchprev(virt_t *rip, virt_t *rsp)
{
    tcb_t *tcb = (tcb_t*)0;
    tcb_t *tcbq = (tcb_t*)&tmp3map;
    pid_t p = tcb->prev;
    int i;

    if(p == 0) {
        /* find previous higher priority queue */
        if(tcb->state == tcb_state_running) {
            for(i=tcb->priority-1;i>=PRI_SYS && !p;i--)
                p = ccb.hd_active[i];
            /* go to blocked queue if none */
            if(!p)
                p = ccb.hd_blocked;
        }
        /* if even blocked was empty, find the last from lowest priority queue */
        if(!p)
            for(i=PRI_IDLE;i>PRI_SYS && !p;i--)
                p = ccb.hd_active[i];
        if(p) {
            //find end of queue
            kmap((virt_t)&tmp3map, p * __PAGESIZE, PG_CORE_NOCACHE);
            sys_fault = false;
            while(sys_fault==false && tcbq->next!=0) {
                p=tcbq->next;
                kmap((virt_t)&tmp3map, p * __PAGESIZE, PG_CORE_NOCACHE);
            }
        }
    }
    if(p) {
        kmap((virt_t)&tmp3map, p * __PAGESIZE, PG_CORE_NOCACHE);
        tcb->rip = *rip;
        task_map(tcbq->memroot);
        *rip = tcb->rip;
        *rsp = tcb->rsp;
    }
}

// switch to the next task
void dbg_switchnext(virt_t *rip, virt_t *rsp)
{
    tcb_t *tcb = (tcb_t*)0;
    tcb_t *tcbq = (tcb_t*)&tmp3map;
    pid_t n = tcb->next;
    int i;
    if(n == 0) {
        /* find the next non empty lower priority queue */
        if(tcb->state == tcb_state_running) {
            for(i=tcb->priority+1;i<=PRI_IDLE && !n;i++)
                n = ccb.hd_active[i];
            /* go to blocked queue if there was none */
            if(!n)
                n = ccb.hd_blocked;
        }
        /* if blocked queue was empty too, find the first highest priority queue */
        if(!n)
            for(i=PRI_SYS;i<PRI_IDLE && !n;i++)
                n = ccb.hd_active[i];
    }
    if(n) {
        kmap((virt_t)&tmp3map, n * __PAGESIZE, PG_CORE_NOCACHE);
        tcb->rip = *rip;
        task_map(tcbq->memroot);
        *rip = tcb->rip;
        *rsp = tcb->rsp;
    }
}

virt_t dbg_getaddr(char *cmd, size_t size)
{
    tcb_t *tcb = (tcb_t*)0;
    uint64_t base = 0,ret = 0, ts=0, ind=0;
    if(*cmd=='*') { cmd++; ind=1; }
    char *s=cmd;
    if(*cmd<'0'||*cmd>'9') {
        while((uint64_t)cmd < (uint64_t)s+size && *cmd!=0 && *cmd != ' ' && *cmd != '-' && *cmd != '+') cmd++;
        /* translate registers */
        ts = (uint64_t)cmd-(uint64_t)s;
        if(ts==3 && s[0]=='c' && s[1]=='r' && s[2]=='2') base = cr2; else
        if(ts==3 && s[0]=='c' && s[1]=='r' && s[2]=='3') base = cr3; else
        if(ts==3 && s[0]=='r' && s[1]=='a' && s[2]=='x') base = *((uint64_t*)(tcb->gpr+0)); else
        if(ts==3 && s[0]=='r' && s[1]=='b' && s[2]=='x') base = *((uint64_t*)(tcb->gpr+8)); else
        if(ts==3 && s[0]=='r' && s[1]=='c' && s[2]=='x') base = *((uint64_t*)(tcb->gpr+16)); else
        if(ts==3 && s[0]=='r' && s[1]=='d' && s[2]=='x') base = *((uint64_t*)(tcb->gpr+24)); else
        if(ts==3 && s[0]=='r' && s[1]=='s' && s[2]=='i') base = *((uint64_t*)(tcb->gpr+32)); else
        if(ts==3 && s[0]=='r' && s[1]=='d' && s[2]=='i') base = *((uint64_t*)(tcb->gpr+40)); else
        if(ts==2 && s[0]=='r' && s[1]=='8') base = *((uint64_t*)(tcb->gpr+48)); else
        if(ts==2 && s[0]=='r' && s[1]=='9') base = *((uint64_t*)(tcb->gpr+56)); else
        if(ts==3 && s[0]=='r' && s[1]=='1' && s[2]=='0') base = *((uint64_t*)(tcb->gpr+64)); else
        if(ts==3 && s[0]=='r' && s[1]=='1' && s[2]=='1') base = *((uint64_t*)(tcb->gpr+72)); else
        if(ts==3 && s[0]=='r' && s[1]=='1' && s[2]=='2') base = *((uint64_t*)(tcb->gpr+80)); else
        if(ts==3 && s[0]=='r' && s[1]=='1' && s[2]=='3') base = *((uint64_t*)(tcb->gpr+88)); else
        if(ts==3 && s[0]=='r' && s[1]=='1' && s[2]=='4') base = *((uint64_t*)(tcb->gpr+96)); else
        if(ts==3 && s[0]=='r' && s[1]=='1' && s[2]=='5') base = *((uint64_t*)(tcb->gpr+104)); else
        if(ts==3 && s[0]=='r' && s[1]=='b' && s[2]=='p') base = *((uint64_t*)(tcb->gpr+112)); else
        /* neither, lookup sym */
            base = elf_lookupsym((uchar*)s,ts);
    }
    /* parse displacement */
    while(*cmd==' '||*cmd=='-'||*cmd=='+') cmd++;
    env_hex((unsigned char*)cmd, (uint64_t*)&ret, 0, 0);
    if(*(cmd-1)=='-') {
        base-=ret;
    } else {
        base+=ret;
    }
    if(ind) {
        base=*((uint64_t*)base);
    }
    return base;
}

#define SINGLESTEP_NONE 0
#define SINGLESTEP_LOCAL 1
#define SINGLESTEP_GLOBAL 2

void dbg_singlestep(uint8_t enable)
{
    tcb_t *tcb=(tcb_t*)&tmp3map;
    int i;
    pid_t n;
    // enable locally
    if(enable) {
        *((uint64_t*)(ccb.ist3+ISR_STACK-24)) |= (1<<8);
        ((tcb_t*)0)->rflags |= (1<<8);
    } else {
        *((uint64_t*)(ccb.ist3+ISR_STACK-24)) &= ~(1<<8);
        ((tcb_t*)0)->rflags &= ~(1<<8);
    }
    // enable globally or clear all flags
    if(enable!=SINGLESTEP_LOCAL) {
        for(i=PRI_SYS; i<PRI_IDLE; i++) {
            if(ccb.hd_active[i] == 0)
                continue;
            n = ccb.hd_active[i];
            while(n != 0) {
                kmap((virt_t)&tmp3map, n * __PAGESIZE, PG_CORE_NOCACHE);
                if(enable!=SINGLESTEP_NONE)
                    tcb->rflags |= (1<<8);
                else
                    tcb->rflags &= ~(1<<8);
                n = tcb->next;
            }
        }
        n = ccb.hd_blocked;
        while(n != 0) {
            kmap((virt_t)&tmp3map, n * __PAGESIZE, PG_CORE_NOCACHE);
            if(enable!=SINGLESTEP_NONE)
                tcb->rflags |= (1<<8);
            else
                tcb->rflags &= ~(1<<8);
            n = tcb->next;
        }
    }
}

/**
 * initialize debugger. Called from sys_enable()
 */
void dbg_init()
{
    int i;
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
    /* clear history */
    for(i=0;i<sizeof(cmdhist);i++) cmdhist[i]=0;

    /* initialize uart as an alternative "keyboard" */
    __asm__ __volatile__ (
        "movw $0x3f9, %%dx;"
        "xorb %%al, %%al;outb %%al, %%dx;"               //IER int off
        "movb $0x80, %%al;addb $2,%%dl;outb %%al, %%dx;" //LCR set divisor mode
        "movb $1, %%al;subb $3,%%dl;outb %%al, %%dx;"    //DLL divisor lo 115200
        "xorb %%al, %%al;incb %%dl;outb %%al, %%dx;"     //DLH divisor hi
        "xorb %%al, %%al;incb %%dl;outb %%al, %%dx;"     //FCR fifo off
        "movb $0x43, %%al;incb %%dl;outb %%al, %%dx;"    //LCR 8N1, break on
        "movb $0x8, %%al;incb %%dl;outb %%al, %%dx;"     //MCR Aux out 2
        "xorb %%al, %%al;subb $4,%%dl;outb %%al, %%dx;inb %%dx, %%al"   //clear receiver/transmitter
    :::"%rax","%rdx");
}

/**
 * activate debugger. Called from ISRs
 */
void dbg_enable(virt_t rip, virt_t rsp, char *reason)
{
    tcb_t *tcb = (tcb_t*)0;
    tcb_t *tcbq = (tcb_t*)&tmp3map;
    uint64_t scancode = 0, offs, line, x, y;
    font_t *font = (font_t*)&_binary_font_start;
    char *tabs[] = { "Code", "Data", "Messages", "TCB", "CCB", "RAM", "Sysinfo", "Syslog" };
    char cmd[64], c;
    int cmdidx=0,cmdlast=0,currhist=sizeof(cmdhist),dbg_bt=0;
    uint8_t m,l;

    // turn of scroll
    scry = -1;
    // set up variables and stack
    __asm__ __volatile__ ( "movq %%cr2, %%rax; movq %%rax, %0" : "=r"(cr2) : : "%rax");
    __asm__ __volatile__ ( "movq %%cr3, %%rax; movq %%rax, %0" : "=r"(cr3) : : "%rax");
    __asm__ __volatile__ ( "movq %%dr6, %%rax; movq %%rax, %0;": "=r"(dr6) : : "%rax");
    __asm__ __volatile__ ( "xorq %%rax, %%rax; movq %%rax,%%dr6": : : "%rax");
    oldist1 = ccb.ist1;
    ccb.ist1 = (uint64_t)safestack + (uint64_t)__PAGESIZE-2048;

    // get rip and rsp to display
    if(rip == 0)
        rip = tcb->rip;
    if(rsp == 0)
        rsp = tcb->rsp;

    dbg_origrip = rip;
    dbg_logpos = 0;
    dbg_next = 0;
    dbg_isshft = false;
    dbg_indump = true;

    if(dbg_start+32<rip) {
        dbg_start=rip;
        do {
            dbg_start--;
        } while(dbg_start>dbg_start-15 && disasm(dbg_start,NULL) != rip);
    }
    dbg_mqptr = *((uint64_t*)MQ_ADDRESS) - 1;
    if(dbg_mqptr==0)
        dbg_mqptr = *((uint64_t*)(MQ_ADDRESS+16)) - 1;

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
        bg = dbg_theme[1];
        offs = font->height*bootboot.fb_scanline*2;
        for(y=font->height;y<(maxy-2)*font->height;y++){
            line = offs;
            for(x=0; x < bootboot.fb_width; x++){
                *((uint32_t*)(FBUF_ADDRESS + line)) = (uint32_t)bg;
                line += 4;
            }
            offs += bootboot.fb_scanline;
        }
        // draw tab
        fx=kx=2; ky=3; fg = dbg_theme[3];
        if(dbg_tui)
            dbg_setpos();
        __asm__ __volatile__ ("pushq %%rdi":::"%rdi");
        if(dbg_tab==tab_code) dbg_code(rip, tcb->rsp, &dbg_bt); else
        if(dbg_tab==tab_data) dbg_data(rsp); else
        if(dbg_tab==tab_tcb) dbg_tcb(); else
        if(dbg_tab==tab_msg) dbg_msg(); else
        if(dbg_tab==tab_queues) dbg_queues(); else
        if(dbg_tab==tab_ram) dbg_ram(); else
        if(dbg_tab==tab_sysinfo) dbg_sysinfo();
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
        kx = maxx-5 - (tcb->pid>0xff?(tcb->pid>0xffff?8:2):1) 
            - ((char*)tcb->name==NULL?4:kstrlen((char*)tcb->name));
        ky = maxy-1;
        scry = -1;
        if(dbg_tui) {
            dbg_setpos();
            dbg_putchar(27);
            dbg_putchar('[');
            dbg_putchar('2');
            dbg_putchar('K');
        }
        kprintf("%s %x ", tcb->name, tcb->pid);
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
            dbg_isshft=false;
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
                    if(scancode=='a') {
                        scancode=328;
                        dbg_isshft=true;
                    } else
                    if(scancode=='B')
                        scancode=336;
                    else
                    if(scancode=='b') {
                        scancode=336;
                        dbg_isshft=true;
                    } else
                    if(scancode=='C')
                        scancode=333;
                    else
                    if(scancode=='D')
                        scancode=331;
                    else
                    if(scancode=='Z') {
                        scancode=15;
                        dbg_isshft=true;
                    } else
                    if(scancode=='3') {
                        scancode = kwaitkey();
                        if(scancode=='~')
                            scancode=339;
                    } else
                    if(scancode=='5') {
                        scancode = kwaitkey();
                        if(scancode=='~')
                            scancode=329;
                    } else
                    if(scancode=='6') {
                        scancode = kwaitkey();
                        if(scancode=='~')
                            scancode=337;
                    } else
                    if(scancode=='1') {
                        scancode = kwaitkey();
                        if(scancode=='1') {
                            scancode = kwaitkey();
                            if(scancode=='~')
                                scancode=59;
                        } else
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
                if(cmdlast==0)
                    goto quitdbg;
                cmdidx = cmdlast = 0;
                cmd[cmdidx]=0;
                goto getcmd;
            }
            // Tab
            case 15: {
                if(dbg_isshft){
                    if(dbg_tab==0)
                        dbg_tab=tab_last-1;
                    else
                        dbg_tab--;
                } else {
                    dbg_tab++;
                    if(dbg_tab>=tab_last)
                        dbg_tab=0;
                }
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
                if(!dbg_isshft) {
                    if(currhist>0 && cmdhist[currhist-sizeof(cmd)]!=0) {
                        currhist-=sizeof(cmd);
                        kmemcpy(&cmd[0],&cmdhist[currhist],sizeof(cmd));
                        cmdidx=cmdlast=kstrlen(cmd);
                    }
                } else {
                    if(dbg_tab == tab_data) {
                        rsp -= (dbg_unit==4? 8 : 16);
                        goto redraw;
                    } else
                    if(dbg_tab == tab_code) {
                        dbg_next = dbg_start;
                        do {
                            dbg_start--;
                        } while(dbg_start>dbg_next-15 && disasm(dbg_start,NULL) != dbg_next);
                        goto redraw;
                    } else
                    if(dbg_tab == tab_ram) {
                        if(dbg_scr>0)
                            dbg_scr--;
                        goto redraw;
                    } else
                    if(dbg_tab == tab_msg) {
                        if(dbg_mqptr > 1)
                            dbg_mqptr--;
                        goto redraw;
                    } else
                    if(dbg_tab == tab_sysinfo) {
                        if(dbg_logpos>0) {
                            dbg_logpos-=2;
                            for(;syslog_buf[dbg_logpos]!='\n' && dbg_logpos>0;dbg_logpos--);
                            if(dbg_logpos>0) dbg_logpos++;
                            goto redraw;
                        }
                    }
                }
                goto getcmd;
            }
            // Down
            case 336: {
                if(!dbg_isshft) {
                    if(currhist<sizeof(cmdhist)) {
                        currhist+=sizeof(cmd);
                        if(currhist<=sizeof(cmdhist)-sizeof(cmd)){
                            kmemcpy(&cmd[0],&cmdhist[currhist],sizeof(cmd));
                            cmdidx=cmdlast=kstrlen(cmd);
                        } else {
                            cmdidx=cmdlast=0;
                            cmd[cmdidx]=0;
                        }
                    }
                } else {
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
                    } else
                    if(dbg_tab == tab_msg) {
                        if(dbg_mqptr < *((uint64_t*)(MQ_ADDRESS+16)))
                            dbg_mqptr++;
                        goto redraw;
                    } else
                    if(dbg_tab == tab_sysinfo) {
                        if(syslog_buf[dbg_logpos]!=0&&syslog_buf[dbg_logpos+1]!=0) {
                            for(x=dbg_logpos;syslog_buf[dbg_logpos]!='\n' && syslog_buf[dbg_logpos]!=0;dbg_logpos++);
                            if(syslog_buf[dbg_logpos]=='\n') dbg_logpos++;
                            if(syslog_buf[dbg_logpos]==0) dbg_logpos=x;
                            goto redraw;
                        }
                    }
                }
                goto getcmd;
            }
            // PgUp
            case 329: {
                if(dbg_tab == tab_data) {
                    int s=dbg_isshft? __SLOTSIZE : __PAGESIZE;
                    rsp--;
                    rsp = (rsp/s)*s;
                    goto redraw;
                }
                if(dbg_tab == tab_code) {
                    if(dbg_bt>0) dbg_bt--;
                    rip = dbg_getrip(&dbg_bt);
                    goto redraw;
                }
                goto getcmd;
            }
            // PgDn
            case 337: {
                if(dbg_tab == tab_data) {
                    int s=dbg_isshft? __SLOTSIZE : __PAGESIZE;
                    rsp += s;
                    rsp = (rsp/s)*s;
                    goto redraw;
                }
                if(dbg_tab == tab_code) {
                    dbg_bt++;
                    rip = dbg_getrip(&dbg_bt);
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
            // F1 Help
            case 59: {
help:
                fx = kx = (maxx-54)/2; ky = 2; fg=dbg_theme[5]; bg=dbg_theme[2];
                if(dbg_tui)
                    dbg_setpos();
                kprintf(
                    "                                                      \n"
                    "     ---==<  OS/Z " OSZ_VER " " OSZ_ARCH " Debugger  >==---      \n"
                    "                                                      \n"
                    " Keyboard Shortcuts                                   \n"
                    "  F1 - this help                                      \n"
                    "  Tab - switch panels (shift+ backwards)              \n"
                    "  Esc - exit debugger / clear command                 \n"
                    "  Enter - repeat step instruction / execute command   \n"
                    "  Left / Right - previous-next task / move cursor     \n"
                    "  Up / Down - command history / (shift+ scroll)       \n"
                    "  Space - toggle instruction bytes / mnemonics        \n"
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
                    "  Tcb - show current task's Task Control Block        \n"
                    "  TUi - toggle video terminal support on serial line  \n"
                    "  Messages - list messages in current task's queue    \n"
                    "  All, CCb - show all task queues and CCB             \n"
                    "  Ram - show RAM information and allocation           \n"
                    "  Instruction, Disasm - instruction disassemble       \n"
                    "  Goto X - go to address X                            \n"
                    "  eXamine [/b1w2d4q8s] X - examine memory at X        \n"
                    "  Break [/b12d4q8rwx] X - set a breakpoint at X       \n"
                    "  SYsinfo - system information                        \n"
                    "  Full - toggle full window mode                      \n"
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
                kprintf_unicodetable();
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
                /* save history */
                if(cmdlast && kmemcmp(&cmdhist[sizeof(cmdhist)-sizeof(cmd)],&cmd[0],kstrlen(cmd))) {
                    kmemcpy(&cmdhist[0],&cmdhist[sizeof(cmd)],sizeof(cmdhist)-sizeof(cmd));
                    kmemcpy(&cmdhist[sizeof(cmdhist)-sizeof(cmd)],&cmd[0],sizeof(cmd));
                }
                currhist=sizeof(cmdhist);
                /* parse commands */
                // step, continue
                if(cmdlast==0 || (cmd[0]=='s'&&cmd[1]!='y') || (cmd[0]=='c' && cmd[1]!='c')) {
quitdbg:
                    /* no continue after crash */
                    if(dbg_theme != theme_debug) {
                        dbg_err = "Unable to continue";
                        goto getcmd;
                    }
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
                    dbg_tui = false;
                    dbg_indump = false;
                    ccb.ist1 = oldist1;
                    // clear screen
/*
                    offs = 0;
                    for(y=0;y<bootboot.fb_height;y++){
                        line=offs;
                        for(x=0;x<bootboot.fb_width;x+=2){
                            *((uint64_t*)(FBUF_ADDRESS + line))=(uint64_t)0;
                            line+=8;
                        }
                        offs+=bootboot.fb_scanline;
                    }
*/
                    msg_sends(EVT_DEST(SRV_video) | EVT_FUNC(VID_flush), 0,0,0,0,0,0);
                    dbg_putchar(13);
                    dbg_putchar(10);
                    kprintf_reset();
                    scry = -1;
                    task_map(cr3);
                    return;
                } else
                // reset, reboot
                if(cmd[0]=='r' && cmd[1]=='e'){
                    dbg_tui = false;
                    dbg_indump = false;
                    ccb.ist1 = oldist1;
                    sys_reset();
                } else
                // quit, halt
                if(cmd[0]=='q'||(cmd[0]=='h' && cmd[1]=='a')){
                    dbg_tui = false;
                    dbg_indump = false;
                    ccb.ist1 = oldist1;
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
                        kmap((virt_t)&tmp3map, y * __PAGESIZE, PG_CORE_NOCACHE);
                        if(tcbq->magic == OSZ_TCB_MAGICH) {
                            tcb->rip = rip;
                            task_map(tcbq->memroot);
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
                // sysinfo, syslog
                if(cmd[0]=='l' || (cmd[0]=='s' && cmd[1]=='y')){
                    cmdidx = cmdlast = 0;
                    cmd[cmdidx]=0;
                    dbg_tab = tab_sysinfo;
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
                            /* use rip when no symbol given */
                            if((cmd[x]!='r'&&cmd[x+1]!='i'&&cmd[x+2]!='p'))
                                dbg_err = "Symbol not found";
                            rip = tcb->rip;
                        } else {
                            /* relative position */
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
                    cmdidx = cmdlast = 0;
                    cmd[cmdidx]=0;
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
                    } else if(x==cmdlast) {
                        rsp = tcb->rsp;
                    }
                    y = 0;
                    while(x<cmdlast && cmd[x]==' ') x++;
                    if(x<cmdlast) {
                        if(cmd[x]=='-'||cmd[x]=='+') x++;
                        y = dbg_getaddr(&cmd[x],cmdlast-x);
                        if(y==0 && cmd[x]!='0'){
                            /* use rsp when no symbol given. Also, tcb returns 0, so check it here */
                            if((cmd[x]!='t'&&cmd[x+1]!='c'&&cmd[x+2]!='b')) {
                                if((cmd[x]!='r'&&cmd[x+1]!='s'&&cmd[x+2]!='p'))
                                    dbg_err = "Symbol not found";
                                rsp = tcb->rsp;
                            } else
                                rsp = 0;
                        } else {
                            /* relative position */
                            if(cmd[x-1]=='-')
                                rsp -= y;
                            else if(cmd[x-1]=='+')
                                rsp += y;
                            else
                                rsp = y;
                        }
                    }
                    // dump physical page
                    if(cmd[1]=='p') {
                        kmap((uint64_t)&tmp3map, rsp, PG_CORE_NOCACHE);
                        rsp=(uint64_t)&tmp3map;
                    }
                    cmdidx = cmdlast = 0;
                    cmd[cmdidx]=0;
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
                //is there a place for a new character?
                if(cmdlast>=sizeof(cmd)-1) {
                    dbg_err = "Buffer full";
                    goto getcmd;
                }
                //if we're not appending, move bytes after cursor
                if(cmdidx<cmdlast) {
                    for(x=cmdlast;x>cmdidx;x--)
                        cmd[x]=cmd[x-1];
                }
                //do we need to translate scancode?
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
                // Space with empty command?
                if(c==' ' && cmdlast==0) {
                    dbg_inst = 1 - dbg_inst;
                    goto redraw;
                }
                //do we have a character?
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
