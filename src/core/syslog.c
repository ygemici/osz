/*
 * core/syslog.c
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
 * @brief Early syslog implementation. Shares buffer with "syslog" task
 */

#include <arch.h>

/* if compiled with debug, (platform)/dbg.c also uses sprintf() */

/* re-entrant counter */
extern char reent;
/* argument */
extern uint8_t cnt;
/* for temporary strings */
extern char tmpstr[33];
extern char nullstr[];

/* the buffer */
dataseg char *syslog_buf;
dataseg char *syslog_ptr;

/* simple sprintf implementation (core can't use libc) */
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
    int64_t i=18,s=c<0;
    if(s) c*=-1;
    if(c>99999999999999999LL)
        c=99999999999999999LL;
    tmpstr[i]=0;
    do {
        tmpstr[--i]='0'+(c%10);
        c/=10;
    } while(c!=0&&i>0);
    if(s)
        tmpstr[--i]='-';
    if(cnt>0&&cnt<18) {
        while(i>18-cnt) {
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
    vastart_sprintf(args, fmt);
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

/**
 * early RFC5424 compatible logger
 */
void syslog_early(char* fmt, ...)
{
    valist args;
    vastart(args, fmt);
    uint64_t arg;
    char *p;
    if(fmt==NULL||fmt[0]==0)
        return;

    /* bound check */
    while(syslog_ptr[0]!=0) syslog_ptr++;
    p=syslog_buf; p+=nrlogmax*__PAGESIZE-256;
    if(((uint64_t)syslog_ptr<(uint64_t)syslog_buf) ||
       ((uint64_t)syslog_ptr>=(uint64_t)p)
    ) {
        syslog_ptr = syslog_buf;
    }
#if DEBUG
    char *oldptr = syslog_ptr;
#endif

    /* date time process prefix, decode from BCD */
    p=(char*)&bootboot.datetime;
    syslog_ptr = sprintf(syslog_ptr, "%1x%1x-", p[0], p[1]);
    syslog_ptr = sprintf(syslog_ptr, "%1x-%1xT", p[2], p[3]);
    syslog_ptr = sprintf(syslog_ptr, "%1x:%1x:%1x", p[4], p[5], p[6]);
    *syslog_ptr = (bootboot.timezone<0?'-':'+'); syslog_ptr++;
    syslog_ptr = sprintf(syslog_ptr, "%1d%1d:",
        (bootboot.timezone<0?-bootboot.timezone:bootboot.timezone)/600,
        ((bootboot.timezone<0?-bootboot.timezone:bootboot.timezone)/60)%10);
    syslog_ptr = sprintf(syslog_ptr, "%1d%1d - boot - - - ",
        ((bootboot.timezone<0?-bootboot.timezone:bootboot.timezone)/10)%10,
        (bootboot.timezone<0?-bootboot.timezone:bootboot.timezone)%10);

    /* copy the message */
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
                *syslog_ptr = (char)arg; syslog_ptr++;
                fmt++;
                continue;
            }
            reent++;
            if(fmt[0]=='a') {
                syslog_ptr = sprintf_putascii(syslog_ptr, arg);
            }
            if(fmt[0]=='d') {
                syslog_ptr = sprintf_putdec(syslog_ptr, arg);
            }
            if(fmt[0]=='x') {
                syslog_ptr = sprintf_puthex(syslog_ptr, arg);
            }
            if(fmt[0]=='s') {
                syslog_ptr = sprintf(syslog_ptr, p);
            }
            reent--;
        } else {
put:        *syslog_ptr++ = *fmt;
        }
        fmt++;
    }
    while(*(syslog_ptr-1)=='\n')
        syslog_ptr--;
    *syslog_ptr = '\n';
    syslog_ptr++;
    *syslog_ptr = 0;
#if DEBUG
    for(p=oldptr;p<syslog_ptr;p++)
        dbg_putchar((int)(*p));
#endif
}
