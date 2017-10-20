/*
 * core/env.c
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
 * @brief Core environment parser (see FS0:\BOOTBOOT\CONFIG or /sys/config)
 */

#include <arch.h>

/*** parsed values ***/
uint64_t nrphymax;
uint64_t nrmqmax;
uint64_t nrlogmax;
uint64_t clocksource;
uint64_t debug;
uint64_t display;
uint64_t quantum;
uint8_t syslog;
uint8_t networking;
uint8_t sound;

/*** for overriding default or autodetected values ***/
// platform specific variables
extern unsigned char *platform_parse(unsigned char *env);

/*** value parsing helper functions ***/

/*** get UTC system timestamp from a BCD local date ***/
uint64_t env_getts(char *p, int16_t timezone)
{
    uint64_t j,r=0,y,m,d,h,i,s;
    /* detect BCD and binary formats */
    if(p[0]>=0x20 && p[0]<=0x30 && p[1]<=0x99 &&    //year
       p[2]>=0x01 && p[2]<=0x12 &&                  //month
       p[3]>=0x01 && p[3]<=0x31 &&                  //day
       p[4]<=0x23 && p[5]<=0x59 && p[6]<=0x60       //hour, min, sec
    ) {
        /* decode BCD */
        y = ((p[0]>>4)*1000)+(p[0]&0x0F)*100+((p[1]>>4)*10)+(p[1]&0x0F);
        m = ((p[2]>>4)*10)+(p[2]&0x0F);
        d = ((p[3]>>4)*10)+(p[3]&0x0F);
        h = ((p[4]>>4)*10)+(p[4]&0x0F);
        i = ((p[5]>>4)*10)+(p[5]&0x0F);
        s = ((p[6]>>4)*10)+(p[6]&0x0F);
    } else {
        /* binary */
        y = (p[1]<<8)+p[0];
        m = p[2];
        d = p[3];
        h = p[4];
        i = p[5];
        s = p[6];
    }
    uint64_t md[12] = {31,0,31,30,31,30,31,31,30,31,30,31};
    /* is year leap year? then tweak February */
    md[1]=((y&3)!=0 ? 28 : ((y%100)==0 && (y%400)!=0?28:29));

    // get number of days since Epoch, cheating
    r = 16801; // precalculated number of days (1970.jan.1-2017.jan.1.)
    for(j=2016;j<y;j++)
        r += ((j&3)!=0 ? 365 : ((j%100)==0 && (j%400)!=0?365:366));
    // in this year
    for(j=1;j<m;j++)
        r += md[j-1];
    // in this month
    r += d-1;
    // convert to sec
    r *= 24*60*60;
    // add time with timezone correction to get UTC timestamp
    r += h*60*60 + (timezone + i)*60 + s;
    // we don't honor leap sec here, but this timestamp should
    // be overwritten anyway by SYS_stime with a more accurate value
    return r;
}

/** parse a hex value */
unsigned char *env_hex(unsigned char *s, uint64_t *v, uint64_t min, uint64_t max)
{
    if(*s=='0' && *(s+1)=='x')
        s+=2;
    do{
        *v <<= 4;
        if(*s>='0' && *s<='9')
            *v += (uint64_t)((unsigned char)(*s)-'0');
        else if(*s >= 'a' && *s <= 'f')
            *v += (uint64_t)((unsigned char)(*s)-'a'+10);
        else if(*s >= 'A' && *s <= 'F')
            *v += (uint64_t)((unsigned char)(*s)-'A'+10);
        s++;
    } while((*s>='0'&&*s<='9')||(*s>='a'&&*s<='f')||(*s>='A'&&*s<='F'));
    if(*v < min)
        *v = min;
    if(max!=0 && *v > max)
        *v = max;
    return s;
}

/** parse a decimal value, fallback to hex with 0x prefix */
unsigned char *env_dec(unsigned char *s, uint64_t *v, uint64_t min, uint64_t max)
{
    if(*s=='0' && *(s+1)=='x')
        return env_hex(s+2, v, min, max);
    *v=0;
    do{
        *v *= 10;
        *v += (uint64_t)((unsigned char)(*s)-'0');
        s++;
    } while(*s>='0'&&*s<='9');
    if(*v < min)
        *v = min;
    if(max!=0 && *v > max)
        *v = max;
    return s;
}

/** parse a false boolean that defaults to true */
unsigned char *env_bool(unsigned char *s, uint8_t *v)
{
    *v = !(*s=='0'||*s=='f'||*s=='F'||*s=='d'||*s=='D'||(*s=='o'&&*(s+1)=='f')||(*s=='O'&&*(s+1)=='F'));
    return s+1;
}

/** parse display configuration */
unsigned char *env_display(unsigned char *s)
{
    uint64_t tmp;
    if(*s>='0' && *s<='9') {
        s = env_dec(s, &tmp, 0, 3);
        display = (uint8_t)tmp;
        return s;
    }
    display = DSP_MONO_COLOR;
    // skip separators
    while(*s==' '||*s=='\t')
        s++;
    if(s[0]=='m' && s[1]=='c')  display = DSP_MONO_COLOR;
    if(s[0]=='s' && s[1]=='m')  display = DSP_STEREO_MONO;
    if(s[0]=='a' && s[1]=='n')  display = DSP_STEREO_MONO;  //anaglyph
    if(s[0]=='s' && s[1]=='c')  display = DSP_STEREO_COLOR;
    if(s[0]=='r' && s[1]=='e')  display = DSP_STEREO_COLOR; //real 3D
    while(*s!=0 && *s!='\n')
        s++;
    return s;
}

/** parse debug flags */
unsigned char *env_debug(unsigned char *s)
{
    uint64_t tmp;
    if(*s>='0' && *s<='9') {
        s = env_dec(s, &tmp, 0, 0xFFFF);
        debug = (uint16_t)tmp;
        return s;
    }
    debug = 0;
    while(*s!=0 && *s!='\n') {
        // skip separators
        if(*s==' '||*s=='\t'||*s==',')
            { s++; continue; }
        // terminators
        if((s[0]=='f'&&s[1]=='a') ||
           (s[0]=='n'&&s[1]=='o')) {
            debug = 0;
            break;
        }
        // debug flags
        if(s[0]=='d' && s[1]=='e')              debug |= DBG_DEVICES;
        if(s[0]=='l' && s[1]=='o')              debug |= DBG_LOG;
#if DEBUG
        if(s[0]=='m' && s[1]=='s')              debug |= DBG_MSG;
        if(s[0]=='m' && s[1]=='m')              debug |= DBG_MEMMAP;
        if(s[0]=='t' && s[1]=='a')              debug |= DBG_TASKS;
        if(s[0]=='e' && s[1]=='l')              debug |= DBG_ELF;
        if(s[0]=='r' && (s[1]=='i'||s[2]=='i')) debug |= DBG_RTIMPORT;
        if(s[0]=='r' && (s[1]=='e'||s[2]=='e')) debug |= DBG_RTEXPORT;
        if(s[0]=='i' && s[1]=='r')              debug |= DBG_IRQ;
        if(s[0]=='s' && s[1]=='c')              debug |= DBG_SCHED;
        if(s[0]=='p' && s[1]=='m')              debug |= DBG_PMM;
        if(s[0]=='v' && s[1]=='m')              debug |= DBG_VMM;
        if(s[0]=='m' && s[1]=='a')              debug |= DBG_MALLOC;
        if(s[0]=='b' && s[1]=='l')              debug |= DBG_BLKIO;
        if(s[0]=='f' && s[1]=='s')              debug |= DBG_FS;
        if(s[0]=='t' && s[1]=='e')              debug |= DBG_TESTS;
#endif
        s++;
    }
    return s;
}

/*** initialize environment ***/
/**
 * parse architecture independent parameters
 */
void env_init()
{
    unsigned char *env = environment;
    unsigned char *env_end = environment+__PAGESIZE;
    uint64_t tmp;

    // set up defaults
    networking = sound = syslog = true;
    nrphymax = nrlogmax = 8;
    nrmqmax = 1;
    quantum = 100;
    display = DSP_MONO_COLOR;
    debug = DBG_NONE;
    // initialize platform variables
    platform_env();

    // parse ascii text
    while(env < env_end && *env!=0) {
        // skip comments
        if((env[0]=='/'&&env[1]=='/') || env[0]=='#') {
            while(env<env_end && env[0]!=0 && env[0]!='\n')
                env++;
        }
        if(env[0]=='/'&&env[1]=='*') {
            env+=2;
            while(env<env_end && env[0]!=0 && env[-1]!='*' && env[0]!='/')
                env++;
        }
        if(env>=env_end)
            break;
        // number of physical memory fragment pages
        if(!kmemcmp(env, "nrphymax=", 9)) {
            env += 9;
            env = env_dec(env, &nrphymax, 2, 128);
        } else
        // number of message queue pages
        if(!kmemcmp(env, "nrmqmax=", 8)) {
            env += 8;
            env = env_dec(env, &nrmqmax, 1, NRMQ_MAX);
        } else
        // number of syslog buffer pages
        if(!kmemcmp(env, "nrlogmax=", 9)) {
            env += 9;
            env = env_dec(env, &nrlogmax, 4, 128);
        } else
        // disable syslog
        if(!kmemcmp(env, "syslog=", 7)) {
            env += 7;
            env = env_bool(env, &syslog);
        } else
        // disable networking
        if(!kmemcmp(env, "networking=", 11)) {
            env += 11;
            env = env_bool(env, &networking);
        } else
        // disable sound
        if(!kmemcmp(env, "sound=", 6)) {
            env += 6;
            env = env_bool(env, &sound);
        } else
        // maximum timeslice a task can allocate
        // CPU continously in timer interrupts
        if(!kmemcmp(env, "quantum=", 8)) {
            env += 8;
            env = env_dec(env, &tmp, 10, 10000);
            quantum=(uint32_t)tmp;
        } else
        // display layout
        if(!kmemcmp(env, "display=", 8)) {
            env += 8;
            env = env_display(env);
        } else
        // output verbosity level
        if(!kmemcmp(env, "debug=", 6)) {
            env += 6;
            env = env_debug(env);
        } else
            // platform specific keys, such as clocksource
            env = platform_parse(env);
    }
}
