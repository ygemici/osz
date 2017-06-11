/*
 * libc/dbg.c
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
 * @brief Debug output
 */
#include <osZ.h>

/* re-entrant counter */
private char dbg_reent = 0;
/* for temporary strings */
private char dbg_tmpstr[33];
/* argument */
private uint8_t dbg_cnt;

void dbg_printf(char* fmt, ...);

void dbg_putchar(int c)
{
    if(c<8 || c>255)
        c='.';
    __asm__ __volatile__ (
        "movl %0, %%eax;movb %%al, %%bl;outb %%al, $0xe9;"
        "movw $0x3fd,%%dx;1:inb %%dx, %%al;andb $0x40,%%al;jz 1b;"
        "subb $5,%%dl;movb %%bl, %%al;outb %%al, %%dx"
    ::"r"(c):"%rax","%rbx","%rdx");
}

void dbg_putascii(int64_t c)
{
    *((uint64_t*)&dbg_tmpstr) = c;
    dbg_tmpstr[dbg_cnt>0&&dbg_cnt<=8?dbg_cnt:8]=0;
    dbg_printf(&dbg_tmpstr[0]);
}

void dbg_putdec(int64_t c)
{
    int i=12;
    dbg_tmpstr[i]=0;
    do {
        dbg_tmpstr[--i]='0'+(c%10);
        c/=10;
    } while(c!=0&&i>0);
    if(dbg_cnt>0&&dbg_cnt<10) {
        while(i>12-dbg_cnt) {
            dbg_tmpstr[--i]=' ';
        }
    }
    dbg_printf(&dbg_tmpstr[i]);
}

void dbg_puthex(int64_t c)
{
    int i=16;
    dbg_tmpstr[i]=0;
    do {
        char n=c & 0xf;
        dbg_tmpstr[--i]=n<10?'0'+n:'A'+n-10;
        c>>=4;
    } while(c!=0&&i>0);
    if(dbg_cnt>0&&dbg_cnt<=8) {
        while(i>16-dbg_cnt*2) {
            dbg_tmpstr[--i]='0';
        }
    }
    dbg_printf(&dbg_tmpstr[i]);
}

void dbg_printf(char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    uint64_t arg;

    if(fmt==NULL)
    {
        if(dbg_reent)
            fmt = "(null)";
        else
            return;
    }

    while(fmt[0]!=0) {
        // argument access
        if(fmt[0]=='%' && !dbg_reent) {
            fmt++; dbg_cnt=0;
            if(fmt[0]=='%') {
                goto put;
            }
            while(fmt[0]>='0'&&fmt[0]<='9') {
                dbg_cnt *= 10;
                dbg_cnt += fmt[0]-'0';
                fmt++;
            }
            arg = va_arg(args, int64_t);
            if(fmt[0]=='c') {
                dbg_putchar((int)((unsigned char)arg));
                fmt++;
                continue;
            }
            dbg_reent++;
            switch(fmt[0]){
                case 'a': dbg_putascii((int64_t)arg); break;
                case 'c': dbg_putchar((int64_t)arg); break;
                case 'd': dbg_putdec((int64_t)arg); break;
                case 'x': dbg_puthex((int64_t)arg); break;
                case 's': dbg_printf((char*)arg); break;
                default: break;
            }
            dbg_reent--;
        } else {
            // convert utf-8 (at fmt) into unicode (to arg)
put:        arg=(uint64_t)((unsigned char)fmt[0]);
            if((arg & 128) != 0) {
                if((arg & 32) == 0 ) {
                    arg=((fmt[0] & 0x1F)<<6)+(fmt[1] & 0x3F);
                    fmt++;
                } else
                if((arg & 16) == 0 ) {
                    arg=((((fmt[0] & 0xF)<<6)+(fmt[1] & 0x3F))<<6)+(fmt[2] & 0x3F);
                    fmt+=2;
                } else
                if((arg & 8) == 0 ) {
                    arg=((((((fmt[0] & 0x7)<<6)+(fmt[1] & 0x3F))<<6)+(fmt[2] & 0x3F))<<6)+(fmt[3] & 0x3F);
                    fmt+=3;
                } else
                    arg=0;
            }
            // display unicode character and move cursor
            dbg_putchar((int)arg);
        }
        fmt++;
    }
}
