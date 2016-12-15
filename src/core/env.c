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

#include "core.h"

// parsed values
uint __attribute__ ((section (".data"))) nrphymax;
uint __attribute__ ((section (".data"))) nrmqmax;
uint8_t __attribute__ ((section (".data"))) identity;
uint8_t __attribute__ ((section (".data"))) verbose;
uint8_t __attribute__ ((section (".data"))) networking;
uint8_t __attribute__ ((section (".data"))) rescueshell;

void env_init()
{
    unsigned char *env = environment;
    int i=__PAGESIZE;
    networking=1;
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
        // disable networking
        if(!kmemcmp(env, "networking=", 11)) {
            env+=11;
            networking = !(*env=='0'||*env=='f'||*env=='F');
        }
        // rescue shell
        if(!kmemcmp(env, "rescueshell=", 12)) {
            env+=12;
            rescueshell = (*env=='1'||*env=='t'||*env=='T');
        }
        // define identity
        if(!kmemcmp(env, "identity=", 9)) {
            env+=9;
            identity = (*env=='1'||*env=='t'||*env=='T');
        }
#if DEBUG
        // output verbosity level
        if(!kmemcmp(env, "verbose=", 8)) {
            env+=8;
            verbose = (*env>='0'&&*env<='9'?*env - '0':0);
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
