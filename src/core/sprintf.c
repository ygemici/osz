/*
 * core/sprintf.c
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
 * @brief String functions for core
 */

#include <platform.h>

/* re-entrant counter */
extern char reent;
/* for temporary strings */
extern char tmpstr[33];
/* argument */
extern uint8_t cnt;
extern char nullstr[];

typedef unsigned char *valist;
#define vastart(list, param) (list = (((valist)&param) + sizeof(void*)*8))
#define vaarg(list, type)    (*(type *)((list += sizeof(void*)) - sizeof(void*)))

char *sprintf(char *dst,char* fmt, ...);

char *sprintf_putascii(char *dst, int64_t c)
{
    kmemcpy((void*)&tmpstr[0],(void*)&c, 8);
    if(cnt>8) cnt=8;
    tmpstr[cnt+1]=0;
    return sprintf(dst,&tmpstr[0]);
}

char *sprintf_putdec(char *dst, int64_t c)
{
    int i=12;
    tmpstr[i]=0;
    do {
        tmpstr[--i]='0'+(c%10);
        c/=10;
    } while(c!=0&&i>0);
    if(cnt>0&&cnt<10) {
        while(i>12-cnt) {
            tmpstr[--i]=' ';
        }
    }
    return sprintf(dst,&tmpstr[i]);
}

char *sprintf_puthex(char *dst, int64_t c)
{
    int i=16;
    tmpstr[i]=0;
    do {
        char n=c & 0xf;
        tmpstr[--i]=n<10?'0'+n:'A'+n-10;
        c>>=4;
    } while(c!=0&&i>0);
    if(cnt>0&&cnt<=8) {
        while(i>16-cnt*2) {
            tmpstr[--i]='0';
        }
    }
    return sprintf(dst,&tmpstr[i]);
}

char *sprintf(char *dst,char* fmt, ...)
{
    valist args;
    vastart(args, fmt);
    uint64_t arg;
    char *p;

    if(dst==NULL)
        return NULL;
    if(fmt==NULL || (((virt_t)fmt>>32)!=0 && ((virt_t)fmt>>32)!=0xffffffff))
        fmt=nullstr;

    while(fmt[0]!=0) {
        // argument access
        if(fmt[0]=='%' && !reent) {
            fmt++; cnt=0;
            if(fmt[0]=='%') {
                goto put;
            }
            while(fmt[0]>='0'&&fmt[0]<='9') {
                cnt *= 10;
                cnt += fmt[0]-'0';
                fmt++;
            }
            p = *((char**)(args));
            arg = vaarg(args, int64_t);
            if(fmt[0]=='c') {
                *dst = (char)arg; dst++;
                fmt++;
                continue;
            }
            reent++;
            if(fmt[0]=='a') {
                dst = sprintf_putascii(dst, arg);
            }
            if(fmt[0]=='d') {
                dst = sprintf_putdec(dst, arg);
            }
            if(fmt[0]=='x') {
                dst = sprintf_puthex(dst, arg);
            }
            if(fmt[0]=='s') {
                dst = sprintf(dst, p);
            }
            reent--;
        } else {
put:        *dst++ = *fmt;
        }
        fmt++;
    }
    return dst;
}
