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
#include "vfs.h"
#include "fsdrv.h"

uint8_t rootmounted = false;
uint16_t nmtab = 0;
mtab_t *mtab = NULL;

void mtab_init()
{
    fcb_add("/",FCB_TYPE_REG_DIR);
}

/**
 * add a mount point to mount list
 */
int16_t mtab_add(char *dev, char *file, char *opts)
{
    fid_t fd, ff;
    int16_t fs;
    int i;
    if(dev==NULL || dev[0]==0 || file==NULL || file[0]==0) {
        seterr(EINVAL);
        return -1;
    }
    // get device fcb
    fd = fcb_get(dev);
    if(fd == -1) {
        seterr(ENODEV);
        return -1;
    }
    // get mount point fcb
    ff = fcb_get(file);
    if(ff == -1 || fcb[ff].type!=FCB_TYPE_REG_DIR) {
        seterr(ENOTDIR);
        return -1;
    }
    // chicken and egg scenario, need /dev before / mounted
    // also refuse to mount anything before root
    if((fd==ff && rootmounted) || (ff!=ROOTFCB && nmtab==0)) {
        seterr(EINVAL);
        return -1;
    }
    // detect file system
    fs=fsdrv_detect(fd);
    if(fs==-1) {
        // extra check
        if(!rootmounted || nmtab==0)
            exit(1);
        seterr(ENOFS);
        return -1;
    }
    // check if it's already mounted
    for(i=0;i<nmtab;i++) {
        if(mtab[i].mountpoint==ff) {
            fcb[mtab[i].storage].nopen--;
            break;
        }
    }
    if(i==nmtab) {
        nmtab++;
        mtab=(mtab_t*)realloc(mtab, nmtab*sizeof(mtab_t));
        if(mtab==NULL)
            return -1;
        fcb[ff].nopen++;
    }
    mtab[i].storage=fd;
    mtab[i].mountpoint=ff;
    mtab[i].fstype=fs;
    fcb[fd].nopen++;

//dbg_printf("dev '%s' file '%s' opts '%s' fs %d\n",dev,file,opts,fs);
    if(ff==ROOTFCB) {
        // when mounting root, also mount /dev. It's path is hardcoded, cannot be changed in OS/Z
        mtab_add(DEVPATH, DEVPATH, "");
        rootmounted = true;
    }
    return -1;
}

/**
 * remove a mount point from mount list
 */
bool_t mtab_del(char *dev, char *file)
{
    int i;
    char *absdev=canonize(dev), *absmnt=canonize(file);
    if(absdev==NULL && absmnt==NULL) {
        seterr(EINVAL);
        if(absdev!=NULL) free(absdev);
        if(absmnt!=NULL) free(absmnt);
        return false;
    }
    for(i=0;i<nmtab;i++) {
        if( (absdev!=NULL && !strcmp(absdev, fcb[mtab[i].storage].abspath)) ||
            (absmnt!=NULL && !strcmp(absmnt, fcb[mtab[i].mountpoint].abspath))) {
                if(absdev!=NULL) free(absdev);
                if(absmnt!=NULL) free(absmnt);
                nmtab--;
                memcpy(&mtab[i], &mtab[i+1], (nmtab-i)*sizeof(mtab_t));
                mtab=(mtab_t*)realloc(mtab, nmtab*sizeof(mtab_t));
                if(mtab==NULL)
                    return false;
                return true;
        }
    }
    if(absdev!=NULL) free(absdev);
    if(absmnt!=NULL) free(absmnt);
    return false;
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

#if DEBUG
void mtab_dump()
{
    int i;
    dbg_printf("Mounts %d:\n",nmtab);
    for(i=0;i<nmtab;i++) {
        dbg_printf("%3d. %s %s %s\n", i, fcb[mtab[i].storage].abspath,
            mtab[i].fstype==-1?"???":fsdrv[mtab[i].fstype].name,
            fcb[mtab[i].mountpoint].abspath);
    }
}
#endif
