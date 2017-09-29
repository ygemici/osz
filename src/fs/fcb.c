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
#include "mtab.h"
#include "fsdrv.h"

uint64_t nfcb = 0;
fcb_t *fcb = NULL;

/**
 * look up a path in fcb directory
 */
fid_t fcb_get(char *abspath)
{
    fid_t i;
    for(i=0;i<nfcb;i++) {
        if(fcb[i].abspath!=NULL && !strcmp(fcb[i].abspath, abspath))
            return i;
    }
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

/**
 * return an fcb index for an absolute path
 */
fid_t fcb_locate(char *path)
{
    char *abspath=canonize(path,NULL);
    fid_t f=fcb_get(abspath),fd=-1;
    int16_t fs=-1;
    int i,j,l=0,k=0;
    // if not found in cache
    if(f==-1) {
        // first, look at static mounts
        // find the longest match in mtab, root fill always match
        for(i=0;i<nmtab;i++) {
            j=strlen(fcb[mtab[i].mountpoint].abspath);
            if(j>l && !memcmp(fcb[mtab[i].mountpoint].abspath, abspath, j)) {
                l=j;
                k=i;
            }
        }
        // get mount device
        fd=mtab[k].storage;
        // and filesystem driver
        fs=mtab[k].fstype;
        // if the longest match was "/", look for automount too
        if(k==ROOTMTAB && !memcmp(abspath, DEVPATH, sizeof(DEVPATH))) {
            i=sizeof(DEVPATH); while(abspath[i]!='/' && !PATHEND(abspath[i])) i++;
            if(abspath[i]=='/') {
                // okay, the path starts with "/dev/*/" Let's get the device fcb
                abspath[i]=0;
                fd=fcb_get(abspath);
                abspath[i++]='/'; l=i;
                fs=-1;
            }
        }
        // only devices and files can serve as storage
        if(fd>=nfcb || (fcb[fd].type!=FCB_TYPE_DEVICE && fcb[fd].type!=FCB_TYPE_REG_FILE)) {
            free(abspath);
            seterr(ENODEV);
            return -1;
        }
        // detect file system on the fly
        if(fs==-1) {
            fs=fsdrv_detect(fd);
            if(k!=ROOTMTAB && fs!=-1)
                mtab[k].fstype=fs;
        }
        // if file system unknown, not much we can do
        if(fs==-1) {
            free(abspath);
            seterr(ENOFS);
            return -1;
        }
        // pass the remaining path to filesystem driver
        // fd=device fcb, fs=fsdrv idx, l=longest mount length, 
dbg_printf("locate dev '%s' mountpoint '%s' fstype '%s' path '%s'\n",
fcb[fd].abspath,fcb[mtab[k].mountpoint].abspath,fsdrv[fs].name,abspath+l);
        //in=(*fsdrvs[mtab[fsmtab].fs_type].locate)(&mtab[fsmtab], 0, canonpath+l);

    }
    free(abspath);
    return f;
}

#if DEBUG
void fcb_dump()
{
    uint64_t i;
    dbg_printf("File Control Blocks %d:\n",nfcb);
    for(i=0;i<nfcb;i++) {
        dbg_printf("%3d. %3d %01x %s\n",i,fcb[i].nopen,fcb[i].type,fcb[i].abspath);
    }
}
#endif
