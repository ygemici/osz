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

#include "arch.h"
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
//extern ccb_t ccb;
extern pmm_t pmm;
extern uint64_t *safestack;
extern uint8_t sys_fault;
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
uint64_t lastsym;
uint8_t dbg_enabled = 0;
uint8_t dbg_tab;
uint8_t dbg_inst;
uint8_t dbg_full;
uint8_t dbg_unit;
uint8_t dbg_numbrk;
uint8_t dbg_iskbd;
uint8_t dbg_isshft;
uint8_t dbg_indump = 0;
uint8_t dbg_tui;
uint64_t dbg_scr;
uint64_t dbg_mqptr;
uint64_t cr2;
uint64_t cr3;
uint64_t dr6;
uint64_t oldist1;
uint64_t dbg_logpos;
char dbg_instruction[128];
virt_t dbg_comment;
virt_t dbg_next;
virt_t dbg_start;
virt_t dbg_lastrip;
virt_t dbg_origrip;
char *dbg_err;
uint32_t *dbg_theme;
uint32_t theme_panic[] = {0x100000,0x400000,0x800000,0x9c3c1b,0xcc6c4b,0xec8c6b} ;
uint32_t theme_debug[] = {0x000020,0x000040,0x101080,0x3e32a2,0x7c71da,0x867ade} ;
//uint32_t theme_debug[] = {0x000010,0x000020,0x000040,0x1b3c9c,0x4b6ccc,0x6b8cec} ;
char *brk = "BRK? ?? set at ????????????????h";
char cmdhist[512];
/*
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
char *prio[] = {
    "SYSTEM   ",
    "RealTime ",
    "Driver   ",
    "Service  ",
    "Important",
    "Normal   ",
    "Low prio ",
    "IDLE ONLY"
};
*/
void dbg_putchar(uint8_t c)
{
    if(c<8 || c>255 || (dbg_indump==2 && (c<' '||c>=127)) || (c>=127&&c<160) || c==173)
        c='.';
    if(c=='\n')
        platform_dbgputc('\r');
    platform_dbgputc(c);
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

/**
 * initialize debugger. Called from sys_enable()
 */
void dbg_init()
{
    int i;
    if(dbg_enabled)
        return;
    dbg_enabled = true;
/*
    dbg_err = NULL;
    dbg_theme = theme_debug;
    dbg_unit = 0;
    dbg_start = 0;
    dbg_lastrip = 0;
    dbg_tab = tab_code;
    dbg_inst = 1;
    dbg_full = 0;
    dbg_scr = 0;
    dbg_indump = 0;
*/
    /* clear history */
    for(i=0;i<sizeof(cmdhist);i++) cmdhist[i]=0;
}

/**
 * activate debugger. Called from ISRs
 */
void dbg_enable(virt_t rip, virt_t rsp, char *reason)
{
}

#endif
