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

extern uint8_t ackdelayed;      // flag to indicate async block read
extern fid_t lookup(char *abspath, bool_t creat);

public uint64_t nfcb = 0;
public uint64_t nfiles = 0;
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
    nfiles++;
    if(j==-1) {
        nfcb++;
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
        if(fcb[idx].type==FCB_TYPE_UNION && fcb[idx].uni.unionlist!=NULL) {
            free(fcb[idx].uni.unionlist);
            fcb[idx].uni.unionlist=NULL;
        }
        if(fcb[idx].abspath!=NULL)
            free(fcb[idx].abspath);
        fcb[idx].abspath=NULL;
        nfiles--;
        idx=nfcb;
        while(idx>0 && fcb[idx-1].abspath==NULL) idx--;
        if(idx!=nfcb) {
            nfcb=idx;
            fcb=(fcb_t*)realloc(fcb, nfcb*sizeof(fcb_t));
        }
    }
}

/**
 * build a list of fids for union
 */
void fcb_unionlist_build(fid_t idx)
{
    int i,j,k,l,n;
    uint64_t bs;
    char *ptr;
    fid_t f,*fl=NULL;
    // failsafe
#if DEBUG
dbg_printf("fcb_unionlist_build %d\n",idx);
#endif
    if(idx>=nfcb || fcb[idx].abspath==NULL || fcb[idx].type!=FCB_TYPE_UNION || 
      fcb[idx].uni.fs==-1 ||fcb[idx].uni.fs>=nfsdrv)
        return;
//    if(fcb[idx].uni.unionlist!=NULL)
//        free(fcb[idx].uni.unionlist);
    bs=fcb[fcb[idx].uni.storage].device.blksize;
    // call file system driver
    ptr=(*fsdrv[fcb[idx].uni.fs].read)(fcb[idx].uni.storage, fcb[idx].uni.inode, 0, &bs);
    // ptr should not be NULL, as union data inlined in inode and inode is in the cache already
    if(ptr==NULL || ptr[0]==0 || ackdelayed)
        return;
    n=1;
    for(i=0;i<bs;i++){
        l=strlen(ptr+i);
        k=-1;
        if(l>3) {
            for(j=0;j<l-3;j++) {
                if(ptr[i+j]=='.' && ptr[i+j+1]=='.' && ptr[i+j+2]=='.') {
                    k=j;
                    break;
                }
            }
        }
        // if union member has joker
        if(k!=-1) {
            ptr[i+k]=0;
#if DEBUG
dbg_printf("  unionlist joker '%s' '%s'\n",ptr+i,ptr+i+k+4);
#endif
            f=lookup(ptr+i,false);
            ptr[i+k]='.';
            if(ackdelayed) return;
            if(f!=-1) {
                //TODO: read direnties from f, iterate on all subdirs with lookup((ptr+i)+d->d_name+(ptr+i+k+4));
                //if lookup!=-1, add to fl
            }
        } else {
            f=lookup(ptr+i,false);
#if DEBUG
dbg_printf("  unionlist '%s' %d\n",ptr+i,f);
#endif
            if(ackdelayed) return;
            if(f!=-1) {
                fl=(fid_t*)realloc(fcb[idx].uni.unionlist,++n*sizeof(fid_t));
                if(fl==NULL) return;
                fl[n-2]=f;
            }
        }
        i+=l;
    }
    fcb[idx].uni.unionlist=fl;
#if DEBUG
    fcb_dump();
#endif
    return;
}

#if DEBUG
void fcb_dump()
{
    uint64_t i,j;
    char *types[]={"file", "dir ", "uni ", "dev ", "pipe", "sock"};
    dbg_printf("File Control Blocks %d:\n",nfcb);
    for(i=0;i<nfcb;i++) {
        dbg_printf("%3d. %3d %s %4d ",i,fcb[i].nopen,types[fcb[i].type],fcb[i].type!=FCB_TYPE_UNION?fcb[i].reg.blksize:0);
        dbg_printf("%3d:%3d %8d %s",fcb[i].reg.storage,fcb[i].reg.inode,fcb[i].reg.filesize,fcb[i].abspath);
        if(fcb[i].type==FCB_TYPE_UNION && fcb[i].uni.unionlist!=NULL) {
            dbg_printf("\t[");
            for(j=0;fcb[i].uni.unionlist[j]!=0;j++)
                dbg_printf(" %d",fcb[i].uni.unionlist[j]);
            dbg_printf(" ]");
        }
        dbg_printf("\n");
    }
}
#endif
