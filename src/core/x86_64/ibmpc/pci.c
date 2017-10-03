/*
 * core/x86_64/ibmpc/pci.c
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
 * @brief PCI functions and enumeration
 */

#include "../arch.h"

extern char *drvs;
extern char *drvs_end;
extern char fn[];

/**
 * find a shared object for a pci address
 */
char *pci_getdriver(char *device)
{
    char *c, *f;
    int d = kstrlen(device);
    for(c=drvs;c<drvs_end;) {
        f = c; while(c<drvs_end && *c!=0 && *c!='\n') c++;
        // skip filesystem drivers
        if(f[0]=='p' && f[1]=='c' && (f[2]!='i')) {
            f+=3;
            if(c-f<255-8 && *(f+d)==9 && !kstrcmp(device,f)) {
                f += d+1;
                kmemcpy(&fn[8], f, c-f);
                fn[c-f+8]=0;
                return fn;
            }
            continue;
        }
        // failsafe
        if(c>=drvs_end || *c==0) break;
        if(*c=='\n') c++;
    }
    return NULL;
}

/**
 * enumerate PCI bus. Called by sys_init()
 */
void pci_init()
{
// TODO: enumerate bus and load drivers
    if(debug&DBG_DEVICES) {
        kprintf("Enumerating PCI bus...\n");
    }
}
