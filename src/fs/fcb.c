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
#include "vfs.h"

extern uint8_t ackdelayed;      // flag to indicate async block read

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
    if(type<FCB_TYPE_PIPE)
        fcb[i].fs=(uint16_t)-1;
    return i;
}

/**
 * remove an fcb entry
 */
void fcb_del(fid_t idx)
{
    int i;
    // failsafe
    if(idx>=nfcb || fcb[idx].abspath==NULL)
        return;
    // decrease link counter
    if(fcb[idx].nopen>0)
        fcb[idx].nopen--;
    // remove if reached zero
    if(fcb[idx].nopen==0) {
        if(fcb[idx].type==FCB_TYPE_UNION && fcb[idx].uni.unionlist!=NULL) {
            for(i=0;fcb[idx].uni.unionlist[i]!=0;i++)
                if(idx!=fcb[idx].uni.unionlist[i])
                    fcb_del(fcb[idx].uni.unionlist[i]);
            free(fcb[idx].uni.unionlist);
            fcb[idx].uni.unionlist=NULL;
        }
        if(fcb[idx].abspath!=NULL)
            free(fcb[idx].abspath);
        fcb[idx].abspath=NULL;
        fcb[idx].reg.filesize=0;
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
 * delete all unused entries from fcb cache
 */
void fcb_cleanup()
{
    int i;
    bool_t was=true;
    while(was) {
        was=false;
        for(i=0;i<nfcb;i++)
            if(fcb[i].nopen==0) {
                was=true;
                fcb_del(i);
            }
    }
}

/**
 * add a fid_t to a fid_t list if it's not already in the list
 */
size_t fcb_unionlist_add(fid_t **fl,fid_t f,size_t n)
{
    int i;
    if(f!=-1) {
        for(i=0;i<n;i++)
            if((*fl)[i]==f)
                return n;
        *fl=(fid_t*)realloc(*fl,++n*sizeof(fid_t));
        if(*fl==NULL) return 0;
        (*fl)[n-2]=f;
    }
    return n;
}

/**
 * build a list of fids for union
 */
fid_t fcb_unionlist_build(fid_t idx, void *buf, size_t s)
{
    int i,j,k,l,n;
    uint64_t bs;
    char *ptr,*fn;
    fid_t f,*fl=NULL;
    // failsafe
    if(idx>=nfcb || fcb[idx].abspath==NULL || fcb[idx].type!=FCB_TYPE_UNION || 
      fcb[idx].fs >= nfsdrv || fsdrv[fcb[idx].fs].read==NULL)
        return -1;
//dbg_printf("fcb_unionlist_build(%d,%x,%d)\n",idx,buf,s);
    bs=fcb[fcb[idx].uni.storage].device.blksize;
    // call file system driver
    ptr=(*fsdrv[fcb[idx].fs].read)(fcb[idx].uni.storage, fcb[idx].uni.inode, 0, &bs);
    // ptr should not be NULL, as union data inlined in inode and inode is in the cache already
    if(ptr==NULL || ptr[0]==0 || ackdelayed)
        return -1;
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
            fn=strndup(ptr+i,k+1);
            if(fn==NULL) return -1;
            f=lookup(fn,false);
//dbg_printf("  unionlist '%s'...'%s' %d buf=%x s=%d\n",fn,ptr+i+k+4,f,buf,s);
            if(ackdelayed) { free(fn); return -1; }
            if(f!=-1 && fcb[f].fs<nfsdrv && fsdrv[fcb[f].fs].getdirent!=NULL) {
                if(buf==NULL) return f;
                j=0; l=1;
                fn=realloc(fn,pathmax);
                if(fn==NULL) continue;
                while(j<s && l!=0) {
                    fn[k]=0;
                    memzero(&dirent.d_name,FILENAME_MAX);
                    l=(*fsdrv[fcb[f].fs].getdirent)(buf+j, j, &dirent);
                    if(dirent.d_name[0]!=0 && dirent.d_name[strlen(dirent.d_name)-1]=='/') {
                        pathcat(fn,dirent.d_name);
                        pathcat(fn,ptr+i+k+4);
                        f=lookup(fn,false);
//dbg_printf("  unionlist '%s' %d\n",fn,f);
                        n=fcb_unionlist_add(&fl,f,n);
                        if(!n) return -1;
                    }
                    j+=l;
                }
            }
            free(fn);
        } else {
            f=lookup(ptr+i,false);
//dbg_printf("  unionlist '%s' %d\n",ptr+i,f);
            if(ackdelayed) return -1;
            n=fcb_unionlist_add(&fl,f,n);
            if(!n) return -1;
        }
        i+=l;
    }
    // okay, we are finished, we won't repeatedly call fcb_unionlist_build
    // again so it's safe to increase the open count on list fids.
    if(fl!=NULL && n>1)
        for(i=0;i<n;i++)
            fcb[fl[i]].nopen++;
    fcb[idx].uni.unionlist=fl;
//    fcb_dump();
    return -1;
}

/**
 * write to an fcb, saves changes in a buffer
 */
public bool_t fcb_write(fid_t idx, off_t offs, void *buf, size_t size)
{
    fcb_t *f;
    writebuf_t *w,*nw,*l=NULL;
    size_t s;
    off_t e,we;
    bool_t chk;
    // failsafe
    if(idx==-1 || idx>=nfcb || fcb[idx].type!=FCB_TYPE_REG_FILE)
        return false;
    f=&fcb[idx];
    // failsafe
    if(fcb[idx].reg.storage!=-1 && fsck.dev==f->reg.storage) { seterr(EBUSY); return false; }
    if(!(f->mode & O_WRITE)) { seterr(EACCES); return false; }
    // allocate a new write buffer
    nw=(writebuf_t*)malloc(sizeof(writebuf_t));
    if(nw==NULL)
        return false;
    nw->data=(void*)malloc(size);
    nw->offs=offs;
    nw->size=size;
    e=offs+size;
    if(nw->data==NULL)
        return false;
    // remove old buffers entirely covered by the new one
    l=NULL;
    for(w=f->reg.buf;w!=NULL;w=w->next) {
        we=w->offs+w->size;
        // ##############       offs,e
        //  +--+ +----+         w->offs,we
        if((offs<=w->offs && e>=we) || w->data==NULL) {
            if(l!=NULL) l->next=w->next; else f->reg.buf=w->next;
            if(w->data) { free(w->data); } free(w); w=l;
        }
        l=w;
    }
    l=NULL; chk=false;
    for(w=f->reg.buf;w!=NULL;w=w->next) {
        we=w->offs+w->size;
        // #######              offs,e
        //          +--------+  w->offs,we
        if(e<w->offs) {
            if(l!=NULL) l->next=nw; else f->reg.buf=nw;
            nw->next=w; memcpy(nw->data,buf,size); chk=true;
            break;
        } else
        // ########
        //    +-------+
        if(offs<w->offs && e>=w->offs && e<we) {
            s=we-offs;
            nw->data=(void*)realloc(nw->data,s); if(nw->data==NULL) return false;
            memcpy(nw->data,buf,size); memcpy(nw->data+size,w->data,we-e);
            w->offs=offs; w->size=s; free(w->data); w->data=nw->data; free(nw); chk=true;
            break;
        } else
        //     ####
        //  +--------+
        if(offs>=w->offs && e<=we) {
            memcpy(w->data+offs-w->offs,buf,size); free(nw->data); free(nw); chk=true;
            break;
        } else
        //     ##########
        // +-------+
        if(offs>w->offs && offs<=we && e>we) {
            w->data=(void*)realloc(nw->data,we-offs); if(w->data==NULL) return false;
            memcpy(w->data+offs-w->offs,buf,size); free(nw->data); free(nw); chk=true;
            break;
        } else
        //           ##########
        // +-------+             +----+?
        if(offs>we && offs<=we && e>we && w->next!=NULL) {
            nw->next=w->next; w->next=nw; memcpy(nw->data,buf,size); chk=true;
            break;
        }
    }
    if(!chk) {
#if DEBUG
dbg_printf("fcb_write new buffer NOT ADDED! Should never happen!!!\n");
#endif
        if(l!=NULL) { nw->next=l->next; l->next=nw; memcpy(nw->data,buf,size); chk=true; }
    }
    return chk;
}

/**
 * flush fcb write buffer to it's storage
 */
public bool_t fcb_flush(fid_t idx)
{
    return true;
}

#if DEBUG
void fcb_dump()
{
    uint64_t i,j;
    char *types[]={"file", "dir ", "uni ", "dev ", "pipe", "sock"};
    dbg_printf("File Control Blocks %d:\n",nfcb);
    for(i=0;i<nfcb;i++) {
        dbg_printf("%3d. %3d ",i,fcb[i].nopen);
        if(fcb[i].abspath==NULL) {
            dbg_printf("(free slot)\n");
            continue;
        }
        dbg_printf("%s %4d %8s ",types[fcb[i].type],
            fcb[i].type!=FCB_TYPE_UNION?fcb[i].reg.blksize:0,fcb[i].fs<nfsdrv?fsdrv[fcb[i].fs].name:"nofs");
        dbg_printf("%c%c%c%c ",fcb[i].mode&O_READ?'r':'-',fcb[i].mode&O_READ?'w':'-',fcb[i].mode&O_READ?'a':'-',
            fcb[i].flags&FCB_FLAG_EXCL?'L':'-');
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
