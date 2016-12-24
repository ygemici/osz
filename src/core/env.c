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
 * @brief Core environment parser (see FS0:\BOOTBOOT\CONFIG)
 */

// parsed values
uint __attribute__ ((section (".data"))) nrphymax;
uint __attribute__ ((section (".data"))) nrmqmax;
uint __attribute__ ((section (".data"))) nrirqmax;
uint8_t __attribute__ ((section (".data"))) identity;
uint8_t __attribute__ ((section (".data"))) debug;
uint8_t __attribute__ ((section (".data"))) networking;
uint8_t __attribute__ ((section (".data"))) sound;
uint8_t __attribute__ ((section (".data"))) rescueshell;

// for overriding default or autodetected values
extern uint64_t lapic_addr;
extern uint64_t ioapic_addr;

unsigned char *env_hex(unsigned char *s, uint *v, uint min, uint max)
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
            *s += (uint64_t)((unsigned char)(*s)-'A'+10);
        s++;
    } while((*s>='0'&&*s<='9')||(*s>='a'&&*s<='f')||(*s>='A'&&*s<='F'));
    if(*v < min)
        *v = min;
    if(max!=0 && *v > max)
        *v = max;
    return s;
}

unsigned char *env_dec(unsigned char *s, uint *v, uint min, uint max)
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

unsigned char *env_boolt(unsigned char *s, uint8_t *v)
{
    *v = (*s=='1'||*s=='t'||*s=='T');
    return s+1;
}

unsigned char *env_boolf(unsigned char *s, uint8_t *v)
{
    *v = !(*s=='0'||*s=='f'||*s=='F');
    return s+1;
}

void env_init()
{
    unsigned char *env = environment;
    int i = __PAGESIZE;
    // set up defaults
    networking = sound = true;
    identity = false;
    ioapic_addr = 0;
    nrirqmax = ISR_NUMHANDLER;
    while(i-->0 && *env!=0) {
        // number of physical memory fragment pages
        if(!kmemcmp(env, "nrphymax=", 9)) {
            env += 9;
            env = env_dec(env, &nrphymax, 1, 32);
        } else
        // number of message queue pages
        if(!kmemcmp(env, "nrmqmax=", 8)) {
            env += 8;
            env = env_dec(env, &nrmqmax, 1, NRMQ_MAX);
        } else
        // maximum number of handlers per IRQ
        if(!kmemcmp(env, "nrirqmax=", 9)) {
            env += 9;
            env = env_dec(env, &nrirqmax, 4, 32);
        } else
        // manually override APIC address
        if(!kmemcmp(env, "apic=", 5)) {
            env += 5;
            // we only accept hex value for this parameter
            env = env_hex(env, (uint*)&lapic_addr, 1024*1024, 0);
        } else
        // manually override IOAPIC address
        if(!kmemcmp(env, "ioapic=", 7)) {
            env += 7;
            // we only accept hex value for this parameter
            env = env_hex(env, (uint*)&ioapic_addr, 1024*1024, 0);
        } else
        // disable networking
        if(!kmemcmp(env, "networking=", 11)) {
            env += 11;
            env = env_boolf(env, &networking);
        } else
        // disable sound
        if(!kmemcmp(env, "sound=", 6)) {
            env += 6;
            env = env_boolf(env, &sound);
        } else
        // rescue shell
        if(!kmemcmp(env, "rescueshell=", 12)) {
            env += 12;
            env = env_boolt(env, &rescueshell);
        } else
        // run first time turn on's ask for identity task
        if(!kmemcmp(env, "identity=", 9)) {
            env += 9;
            env = env_boolf(env, &identity);
        } else
#if DEBUG
        // output verbosity level
        if(!kmemcmp(env, "debug=", 6)) {
            env += 6;
            debug = (*env>='0'&&*env<='9'?*env - '0':0);
            env++;
        } else
#endif
            env++;
    }
}
