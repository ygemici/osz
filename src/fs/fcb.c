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
#include "vfs.h"
#include "fsdrv.h"

public uint64_t nfcb = 0;
public fcb_t *fcb = NULL;

/**
 * look up a path in fcb directory
 */
fid_t fcb_get(char *abspath)
{
    fid_t i;
    char *ap=NULL;
    if(abspath==NULL || abspath[0]==0)
        return -1;
    i=strlen(abspath);
    if(abspath[i-1]!='/') {
        ap=(char*)malloc(i+2);
        if(ap!=NULL) {
            memcpy(ap, abspath, i);
            ap[i++]='/'; ap[i]=0;
        }
    }
    for(i=0;i<nfcb;i++) {
        if(fcb[i].abspath!=NULL && (!strcmp(fcb[i].abspath, abspath) || (ap!=NULL && !strcmp(fcb[i].abspath, ap)))) {
            if(ap!=NULL)
                free(ap);
            return i;
        }
    }
    if(ap!=NULL)
        free(ap);
    return -1;
}

/**
 * look up a path in fcb directory and add it if not found
 */
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

/**
 * remove an fcb entry
 */
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
    char *types[]={"file", "dir ", "dev ", "pipe", "sock"};
    dbg_printf("File Control Blocks %d:\n",nfcb);
    for(i=0;i<nfcb;i++) {
        dbg_printf("%3d. %3d %s ",i,fcb[i].nopen,types[fcb[i].type]);
        dbg_printf("%3d:%3d %8d %s\n",fcb[i].reg.storage,fcb[i].reg.inode,fcb[i].reg.filesize,fcb[i].abspath);
    }
}
#endif
