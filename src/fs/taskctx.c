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
#include "taskctx.h"
#include "fcb.h"

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
    if(fid>=nfcb)
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
    if(fid>=nfcb)
        return;
    fcb_del(tc->workdir);
    tc->workdir=fid;
    fcb[fid].nopen++;
}

/**
 * add a file to open file descriptors
 */
uint64_t taskctx_open(taskctx_t *tc, fid_t fid, mode_t mode, fpos_t offs)
{
    uint64_t i;
    if(fid>=nfcb)
        return -1;
    // find an empty slot
    if(tc->openfiles!=NULL) {
        for(i=0;i<tc->nopenfiles;i++)
            if(tc->openfiles[i].fid==-1)
                goto found;
    } else
        tc->nopenfiles=0;
    // if not found, allocate a new slot
    i=tc->nopenfiles;
    tc->openfiles=(openfiles_t*)realloc(tc->openfiles,(tc->nopenfiles+1)*sizeof(openfiles_t));
    if(tc->openfiles==NULL)
        return -1;
    // add new open file descriptor
found:
    tc->openfiles[i].fid=fid;
    tc->openfiles[i].mode=mode;
    tc->openfiles[i].offs=offs;
    tc->nopenfiles++;
    fcb[fid].nopen++;
    return i;
}

/**
 * remove a file from open file descriptors
 */
bool_t taskctx_close(taskctx_t *tc, uint64_t idx)
{
    if(tc->openfiles==NULL || tc->nopenfiles<=idx || tc->openfiles[idx].fid==-1)
        return false;
    fcb_del(tc->openfiles[idx].fid);
    tc->nopenfiles--;
    if(idx<=tc->nopenfiles)
        tc->openfiles[idx].fid=-1;
    else {
        if(tc->nopenfiles==0) {
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
    if(tc->openfiles==NULL || tc->nopenfiles<=idx || tc->openfiles[idx].fid==-1 ||
        fcb[tc->openfiles[idx].fid].type!=FCB_TYPE_REG_FILE)
        return false;
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
