/*
 * fs/mtab.c
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
 * @brief Mount points
 */

#include <osZ.h>
#include "mtab.h"
#include "fcb.h"
#include "devfs.h"

uint8_t rootmounted = false;

void mtab_init()
{
    fcb_add("/",FCB_TYPE_REG_DIR);
}

/**
 * add a mount point to mount list
 */
uint64_t mtab_add(char *dev, char *file, char *opts)
{
    fid_t fd, ff;
    if(dev==NULL || dev[0]==0 || file==NULL || file[0]==0) {
        seterr(EINVAL);
        return -1;
    }
    // get device fcb
    fd = fcb_get(dev);
    if(fd == -1) {
        seterr(EINVAL);
        return -1;
    }
    // get mount point fcb
    ff = fcb_get(file);
    if(ff == -1) {
        seterr(EINVAL);
        return -1;
    }
    // chicken and egg scenario, /dev before /
    if(fd==ff && rootmounted) {
        seterr(EINVAL);
        return -1;
    }
    
    fcb[fd].nopen++;
    fcb[ff].nopen++;

    dbg_printf("dev '%s' file '%s' opts '%s'\n",dev,file,opts);
    if(ff==0) {
        // when mounting root, also mount /dev. It's path is hardcoded, cannot be changed in OS/Z
        mtab_add(DEVPATH, DEVPATH, "");
        rootmounted = true;
    }
    return 0;
}

/**
 * parse /etc/fstab and build file system mount points
 */
void mtab_fstab(char *ptr, size_t size)
{
    char *dev, *file, *opts, *end, pass;
    char *fstab = (char*)malloc(size);
    // create a copy 'cos we are going to convert strings to asciiz in it
    if(!fstab || errno())
        abort();
    memcpy(fstab,ptr,size);
    end=fstab+size;
    // make sure we call mtab_add in order
    for(pass='1';pass<='9';pass++) {
        ptr=fstab;
        while(ptr<end) {
            while(*ptr=='\n'||*ptr=='\t'||*ptr==' ') ptr++;
            // skip comments
            if(*ptr=='#') {
                for(ptr++;ptr<end && *(ptr-1)!='\n';ptr++);
                continue;
            }
            // block device
            dev=ptr;
            while(ptr<end && *ptr!=0 && *ptr!='\t' && *ptr!=' ') ptr++;
            if(*ptr=='\n') continue;
            *ptr=0; ptr++;
            while(ptr<end && (*ptr=='\t' || *ptr==' ')) ptr++;
            if(*ptr=='\n') continue;
            // mount point
            file=ptr;
            while(ptr<end && *ptr!=0 && *ptr!='\t' && *ptr!=' ') ptr++;
            if(*ptr=='\n') continue;
            *ptr=0; ptr++;
            while(ptr<end && (*ptr=='\t' || *ptr==' ')) ptr++;
            if(*ptr=='\n') continue;
            // opts and access
            opts=ptr;
            while(ptr<end && *ptr!=0 && *ptr!='\t' && *ptr!=' ') ptr++;
            if(*ptr=='\n') continue;
            *ptr=0; ptr++;
            while(ptr<end && (*ptr=='\t' || *ptr==' ')) ptr++;
            if(*ptr=='\n') continue;
            // pass
            if(*ptr==pass)
                mtab_add(dev,file,opts);
            ptr++;
        }
        // if we haven't mounted root in the first pass, add initrd manually
        if(pass=='1' && !rootmounted)
            mtab_add("/dev/root", "/", "");
    }
    free(fstab);
}
