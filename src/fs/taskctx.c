/*
 * fs/taskctx.c
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
 * @brief Task context for file system services
 */

#include <osZ.h>
#include <stdio.h>
#include "fcb.h"
#include "fsdrv.h"
#include "taskctx.h"

// union lists
int32_t nunionlist = 0;
fid_t **unionlist = NULL;

// task contexts
uint64_t ntaskctx = 0;
taskctx_t *taskctx[256];

// current context
public taskctx_t *ctx;

/**
 * get a task context (add a new one if not found)
 */
taskctx_t *taskctx_get(pid_t pid)
{
    taskctx_t *tc=taskctx[pid & 0xFF], *ntc;
    // find in hash
    while(tc!=NULL) {
        if(tc->pid==pid)
            return tc;
        tc=tc->next;
    }
    // not found, add
    ntc=(taskctx_t*)malloc(sizeof(taskctx_t));
    if(ntc==NULL)
        return NULL;
    ntc->pid=pid;
    // link
    if(tc!=NULL)
        tc->next=ntc;
    else
        taskctx[pid & 0xFF]=ntc;
    ntc->fs=-1;
    fcb[0].nopen+=2;
    ntaskctx++;
    return ntc;
}

/**
 * remove a task context
 */
void taskctx_del(pid_t pid)
{
    taskctx_t *tc=taskctx[pid & 0xFF], *ltc=NULL;
    uint64_t i;
    // failsafe
    if(tc==NULL)
        return;
    // find in hash
    while(tc->next!=NULL) {
        if(tc->pid==pid)
            break;
        ltc=tc;
        tc=tc->next;
    }
    // unlink
    if(ltc==NULL)
        taskctx[pid & 0xFF]=tc->next;
    else
        ltc->next=tc->next;
    ntaskctx--;
    // free memory
    fcb_del(tc->workdir);
    fcb_del(tc->rootdir);
    if(tc->openfiles!=NULL) {
        for(i=0;i<tc->nopenfiles;i++)
            fcb_del(tc->openfiles[i].fid);
        free(tc->openfiles);
    }
    free(tc);
}

/**
 * set root directory
 */
void taskctx_rootdir(taskctx_t *tc, fid_t fid)
{
    if(tc==NULL || fid>=nfcb || fcb[fid].abspath==NULL)
        return;
    fcb_del(tc->rootdir);
    tc->rootdir=fid;
    fcb[fid].nopen++;
}

/**
 * set working directory
 */
void taskctx_workdir(taskctx_t *tc, fid_t fid)
{
    if(tc==NULL || fid>=nfcb || fcb[fid].abspath==NULL)
        return;
    fcb_del(tc->workdir);
    tc->workdir=fid;
    fcb[fid].nopen++;
}

/**
 * add a file to open file descriptors
 */
uint64_t taskctx_open(taskctx_t *tc, fid_t fid, mode_t mode, fpos_t offs, uint64_t idx)
{
    uint64_t i;
    if(tc==NULL || fid>=nfcb || fcb[fid].abspath==NULL) {
        seterr(ENOENT);
        return -1;
    }
    // check if it's already opened for exclusive access
    if(fcb[fid].mode & O_EXCL) {
        seterr(EBUSY);
        return -1;
    }
    // force file descriptor index
    if(idx!=-1 && idx<tc->nopenfiles && tc->openfiles[idx].fid==-1) {
        i=idx;
        goto found;
    }
    // find the first empty slot
    if(tc->openfiles!=NULL) {
        for(i=0;i<tc->nopenfiles;i++)
            if(tc->openfiles[i].fid==-1)
                goto found;
    } else
        tc->nopenfiles=0;
    // if not found, allocate a new slot
    i=tc->nopenfiles;
    tc->nopenfiles++;
    tc->openfiles=(openfiles_t*)realloc(tc->openfiles,tc->nopenfiles*sizeof(openfiles_t));
    if(tc->openfiles==NULL)
        return -1;
    // add new open file descriptor
found:
    // handle exclusive access
    if(mode & O_EXCL)
        fcb[fid].mode |= O_EXCL;
    // deceide operation mode
    if(!(mode & O_READ) && !(mode & O_WRITE))
        mode |= O_READ;
    if(mode & O_CREAT)
        mode |= O_WRITE;
    // save open file descriptor
    tc->openfiles[i].fid=fid;
    tc->openfiles[i].mode=mode;
    tc->openfiles[i].offs=offs;
    tc->openfiles[i].unionidx=0;
    tc->nfiles++;
    fcb[fid].nopen++;
    return i;
}

/**
 * remove a file from open file descriptors
 */
bool_t taskctx_close(taskctx_t *tc, uint64_t idx, bool_t dontfree)
{
    if(!taskctx_validfid(tc,idx))
        return false;
    if(tc->openfiles[idx].mode & O_TMPFILE && fcb[tc->openfiles[idx].fid].mode!=FCB_TYPE_REG_DIR) {
        //unlink()
    } else {
        //cache_flush(tc->openfiles[idx].fid)
        fcb[tc->openfiles[idx].fid].mode &= ~O_EXCL;
        fcb_del(tc->openfiles[idx].fid);
    }
    tc->openfiles[idx].fid=-1;
    tc->nfiles--;
    if(dontfree)
        return true;
    idx=tc->nopenfiles;
    while(idx>0 && tc->openfiles[idx-1].fid==-1) idx--;
    if(idx!=tc->nopenfiles) {
        tc->nopenfiles=idx;
        if(idx==0) {
            free(tc->openfiles);
            tc->openfiles=NULL;
        } else {
            tc->openfiles=(openfiles_t*)realloc(tc->openfiles,tc->nopenfiles*sizeof(openfiles_t));
            if(tc->openfiles==NULL)
                return false;
        }
    }
    return true;
}

/**
 * change position on a open file descriptor
 */
bool_t taskctx_seek(taskctx_t *tc, uint64_t idx, off_t offs, uint8_t whence)
{
    if(!taskctx_validfid(tc,idx)){
        seterr(EBADF);
        return false;
    }
    if(fcb[tc->openfiles[idx].fid].type!=FCB_TYPE_REG_FILE) {
        seterr(ESPIPE);
        return false;
    }
    switch(whence) {
        case SEEK_CUR:
            tc->openfiles[idx].offs += offs;
            break;
        case SEEK_END:
            tc->openfiles[idx].offs = fcb[tc->openfiles[idx].fid].reg.filesize + offs;
            break;
        default:
            tc->openfiles[idx].offs = offs;
            break;
    }
    if((off_t)tc->openfiles[idx].offs<0) {
        tc->openfiles[idx].offs = 0;
        seterr(EINVAL);
        return false;
    }
    return true;
}

/**
 * see if a file descriptor valid for a context
 */
bool_t taskctx_validfid(taskctx_t *tc, uint64_t idx)
{
    return !(tc==NULL || tc->openfiles==NULL || tc->nopenfiles<=idx || tc->openfiles[idx].fid==-1 ||
        fcb[tc->openfiles[idx].fid].abspath==NULL);
}

#if DEBUG
void taskctx_dump()
{
    int i,j;
    taskctx_t *tc;
    dbg_printf("Task Contexts %d:\n",ntaskctx);
    for(i=0;i<256;i++) {
        if(taskctx[i]==NULL)
            continue;
        tc=taskctx[i];
        while(tc!=NULL) {
            dbg_printf(" %x %s%02x ", tc, ctx==tc?"*":" ", tc->pid);
            dbg_printf("root %3d pwd %3d open %2d:", tc->rootdir, tc->workdir, tc->nopenfiles);
            for(j=0;j<tc->nopenfiles;j++)
                dbg_printf(" (%d,%1x,%d)", tc->openfiles[j].fid, tc->openfiles[j].mode, tc->openfiles[j].offs);
            dbg_printf("\n");
            tc=tc->next;
        }
    }
}
#endif
