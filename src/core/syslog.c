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
 * @brief Physical Memory Manager architecture specific part
 */

#include "env.h"

/* re-entrant counter */
extern char reent;
/* argument */
extern uint8_t cnt;

typedef unsigned char *valist;
#define vastart(list, param) (list = (((valist)&param) + sizeof(void*)*6))
#define vaarg(list, type)    (*(type *)((list += sizeof(void*)) - sizeof(void*)))

extern char *sprintf(char *dst,char* fmt, ...);
extern char *sprintf_putascii(char *dst, int64_t c);
extern char *sprintf_putdec(char *dst, int64_t c);
extern char *sprintf_puthex(char *dst, int64_t c);

char __attribute__ ((section (".data"))) *syslog_buf;
char __attribute__ ((section (".data"))) *syslog_ptr;
char __attribute__ ((section (".data"))) syslog_header[] =
    OSZ_NAME " " OSZ_VER "-" OSZ_ARCH " (build " OSZ_BUILD ")\n"
    "Copyright 2017 CC-by-nc-sa bztsrc@github\nUse is subject to license terms.\n\n";

/* early RFC5424 compatible logger */
void syslog_early(char* fmt, ...)
{
    valist args;
    vastart(args, fmt);
    uint64_t arg;
    char *p;

    if(fmt==NULL||fmt[0]==0)
        return;

    /* bound check */
    p=syslog_buf; p+=nrlogmax*__PAGESIZE-256;
    if(((uint64_t)syslog_ptr<(uint64_t)syslog_buf) ||
       ((uint64_t)syslog_ptr>=(uint64_t)p)
    ) {
        syslog_ptr = syslog_buf;
    }

    /* date time process prefix, decode from BCD */
    p=(char*)&bootboot.datetime;
    syslog_ptr = sprintf(syslog_ptr, "%1x%1x-", p[0], p[1]);
    syslog_ptr = sprintf(syslog_ptr, "%1x-%1xT", p[2], p[3]);
    syslog_ptr = sprintf(syslog_ptr, "%1x:%1x:%1x", p[4], p[5], p[6]);
    *syslog_ptr = (bootboot.timezone<0?'-':'+'); syslog_ptr++;
    syslog_ptr = sprintf(syslog_ptr, "%1d%1d:",
        (bootboot.timezone<0?-bootboot.timezone:bootboot.timezone)/600,
        (bootboot.timezone<0?-bootboot.timezone:bootboot.timezone)/60);
    syslog_ptr = sprintf(syslog_ptr, "%1d%1d - boot - - - ",
        (bootboot.timezone<0?-bootboot.timezone:bootboot.timezone)/600,
        (bootboot.timezone<0?-bootboot.timezone:bootboot.timezone)/60);

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
    while(*(syslog_ptr-1)=='\n') syslog_ptr--;
    *syslog_ptr = '\n';
    syslog_ptr++;
    *syslog_ptr = 0;
}

