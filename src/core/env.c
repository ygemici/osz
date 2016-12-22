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
uint8_t __attribute__ ((section (".data"))) identity;
uint8_t __attribute__ ((section (".data"))) debug;
uint8_t __attribute__ ((section (".data"))) networking;
uint8_t __attribute__ ((section (".data"))) sound;
uint8_t __attribute__ ((section (".data"))) rescueshell;

extern uint64_t ioapic_addr;

void env_init()
{
    unsigned char *env = environment;
    int i = __PAGESIZE;
    networking = sound = 1;
    ioapic_addr = 0;
    while(i-->0 && *env!=0) {
        // number of physical memory fragment pages
        if(!kmemcmp(env, "nrphymax=", 9)) {
            env+=9;
            // no atoi() yet
            nrphymax=0;
            do{
                nrphymax*=10;
                nrphymax+=(uint64_t)((unsigned char)(*env)-'0');
                env++;
            } while(*env>='0'&&*env<='9'&&nrphymax<=255);
        }
        // number of message queue pages
        if(!kmemcmp(env, "nrmqmax=", 8)) {
            env+=8;
            // no atoi() yet
            nrmqmax=0;
            do{
                nrmqmax*=10;
                nrmqmax+=(uint64_t)((unsigned char)(*env)-'0');
                env++;
            } while(*env>='0'&&*env<='9'&&nrmqmax<=NRMQ_MAX);
        }
        // manually override IOAPIC address
        if(!kmemcmp(env, "ioapic=", 7)) {
            env+=7;
            // we only accept hex value for this parameter
            if(*env=='0' && *(env+1)=='x')
                env+=2;
            do{
                ioapic_addr<<=4;
                if(*env>='0'&&*env<='9')
                    ioapic_addr+=(uint64_t)((unsigned char)(*env)-'0');
                else if(*env>='a'&&*env<='f')
                    ioapic_addr+=(uint64_t)((unsigned char)(*env)-'a'+10);
                else if(*env>='A'&&*env<='F')
                    ioapic_addr+=(uint64_t)((unsigned char)(*env)-'A'+10);
                env++;
            } while((*env>='0'&&*env<='9')||(*env>='a'&&*env<='f')||(*env>='A'&&*env<='F'));
        }
        // disable networking
        if(!kmemcmp(env, "networking=", 11)) {
            env+=11;
            networking = !(*env=='0'||*env=='f'||*env=='F');
        }
        // disable sound
        if(!kmemcmp(env, "sound=", 6)) {
            env+=6;
            sound = !(*env=='0'||*env=='f'||*env=='F');
        }
        // rescue shell
        if(!kmemcmp(env, "rescueshell=", 12)) {
            env+=12;
            rescueshell = (*env=='1'||*env=='t'||*env=='T');
        }
        // run first time turn on's ask for identity process
        if(!kmemcmp(env, "identity=", 9)) {
            env+=9;
            identity = (*env=='1'||*env=='t'||*env=='T');
        }
#if DEBUG
        // output verbosity level
        if(!kmemcmp(env, "debug=", 6)) {
            env+=6;
            debug = (*env>='0'&&*env<='9'?*env - '0':0);
        }
#endif
        env++;
    }
    // lower bounds
    if(nrphymax<2)
        nrphymax=2;
    if(nrmqmax<1)
        nrmqmax=1;
    // upper bounds
    if(nrphymax>255)
        nrphymax=255;
    if(nrmqmax>NRMQ_MAX)
        nrmqmax=NRMQ_MAX;
}
