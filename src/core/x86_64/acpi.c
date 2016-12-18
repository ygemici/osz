/*
 * core/x86_64/acpi.c
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
 * @brief ACPI table parser
 */

#include "../core.h"

char *acpi_getdriver(char *device, char *drvs, char *drvs_end)
{
    return NULL;
}

void dev_init()
{
    // this is so early, we don't have initrd in fs process' bss yet.
    // so we have to rely on identity mapping to locate the files
    char *s, *f, *drvs = (char *)fs_locate("etc/sys/drivers");
    char *drvs_end = drvs + fs_size;
    char fn[256];

    if(drvs==NULL) {
        // should never happen!
        kprintf("core - W - missing /etc/sys/drivers\n");
        // hardcoded devices if driver list not found
        service_loadso("lib/sys/input/ps2.so");
        service_loadso("lib/sys/display/fb.so");
    } else {
        // load devices which don't have entry in any ACPI tables
        for(s=drvs;s<drvs_end;) {
            f = s; while(s<drvs_end && *s!=0 && *s!='\n') s++;
            // skip filesystem drivers
            if(f[0]=='*' && f[1]==9 && (f[2]!='f' || f[3]!='s')) {
                f+=2;
                if(s-f<255-8) {
                    kmemcpy(&fn[0], "lib/sys/", 8);
                    kmemcpy(&fn[8], f, s-f);
                    fn[s-f+8]=0;
                    service_loadso(fn);
                }
                continue;
            }
            // failsafe
            if(s>=drvs_end || *s==0) break;
            if(*s=='\n') s++;
        }
        // parse ACPI tables to detect devices
    }
}
