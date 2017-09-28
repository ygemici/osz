/*
 * fs/fcb.c
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
 * @brief File Control Block
 */

#include <osZ.h>
#include "fcb.h"

uint64_t nfcb = 0;
fcb_t *fcb = NULL;

fid_t fcb_get(char *abspath)
{
    fid_t i;
    for(i=0;i<nfcb;i++) {
        if(fcb[i].abspath!=NULL && !strcmp(fcb[i].abspath, abspath))
            return i;
    }
    return -1;
}

fid_t fcb_add(char *abspath, uint8_t type)
{
    fid_t i,j=-1;
    for(i=0;i<nfcb;i++) {
        if(fcb[i].abspath!=NULL) {
            if(!strcmp(fcb[i].abspath, abspath))
                return i;
        } else {
            if(j==-1)
                j=i;
        }
    }
    nfcb++;
    if(j==-1) {
        fcb=(fcb_t*)realloc(fcb, nfcb*sizeof(fcb_t));
        if(fcb==NULL)
            return -1;
    } else
        i=j;
    // gcc bug. fcb[i].abspath=strdup(abspath); generates bad code
    abspath=strdup(abspath);
    fcb[i].abspath=abspath;
    fcb[i].type=type;
    return i;
}

void fcb_del(fid_t idx)
{
    // failsafe
    if(idx>=nfcb || fcb[idx].abspath==NULL)
        return;
    // decrease link counter
    fcb[idx].nopen--;
    // remove if reached zero
    if(fcb[idx].nopen==0) {
        if(fcb[idx].abspath!=NULL)
            free(fcb[idx].abspath);
        fcb[idx].abspath=NULL;
        nfcb--;
    }
}
 
#if DEBUG
void fcb_dump()
{
    uint64_t i;
    dbg_printf("File Control Blocks (%d):\n",nfcb);
    for(i=0;i<nfcb;i++) {
        dbg_printf("%3d. %3d %01x %s\n",i,fcb[i].nopen,fcb[i].type,fcb[i].abspath);
    }
}
#endif
