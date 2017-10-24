/*
 * core/security.c
 *
 * Copyright 2017 CC-by-nc-sa bztsrc@github
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
 * @brief Security functions
 */

#include <arch.h>

/* errno in core */
extern uint64_t coreerrno;

/* root uuid, make sure it's padded with zeros */
uint8_t rootuid[15] = { 'r', 'o', 'o', 't', 0,0,0,0,0,0,0,0,0,0,0 };

/**
 * check if current task has a specific Access Control Entry
 */
bool_t task_allowed(tcb_t *tcb, char *grp, uint8_t access)
{
    char *g;
    int i,j;
    if(grp==NULL||grp[0]==0)
        return false;
    j=kstrlen(grp);
    if(j>15)
        j=15;
    for(i=0;i<TCB_MAXACE;i++) {
        g = (char *)(&tcb->acl[i]);
        // failsafe, end of list
        if(g[0]==0)
            return false;
        // if any access requested or has the specific access for the ACE
        if(!kmemcmp(g, grp, j))
            return (access==0 || (g[15] & access)==access) ? true : false;
    }
    return false;
}

/**
 * check if current task can send the message
 */
bool_t msg_allowed(tcb_t *sender, pid_t dest, evt_t event)
{
    evt_t e=EVT_FUNC(event);
    /* emergency task allowed to bypass checks */
    if(sender->priority==PRI_SYS)
        return true;
    // Core services
    if(dest == SRV_CORE) {
        // only drivers can set entries in IRT
        if(e == SYS_setirq && sender->priority != PRI_DRV) goto noaccess;
        // IRQ's SYS_ack to core only allowed for drivers, inlined check in isr_syscall for performance
        /*** service checks ***/
        // root user allowed to call all services
        if(!kmemcmp((void*)&sender->owner, rootuid, 15)) return true;
        if(e == SYS_stime && sender->priority != PRI_DRV && !task_allowed(sender, "stime", A_WRITE)) goto noaccess;
        // other services allowed to everyone
        return true;
    }
    // FS task
    if(dest == SRV_FS || dest == services[-SRV_FS]) {
        // only init allowed to send SYS_mountfs to FS
        if(e==SYS_mountfs && sender->pid!=services[-SRV_init]) goto noaccess;
        // only drivers and system services allowed to send SYS_mknod to FS
        if((e==SYS_mknod || e==SYS_setblock) &&
            sender->priority!=PRI_DRV && sender->priority!=PRI_SRV) goto noaccess;
        /*** service checks ***/
        // root user allowed to call all services
        if(!kmemcmp((void*)&sender->owner, rootuid, 15)) return true;
        if(e==SYS_chroot && !task_allowed(sender, "chroot", A_WRITE)) goto noaccess;
        if((e==SYS_mount||e==SYS_umount) && !task_allowed(sender, "mount", A_WRITE)) goto noaccess;
        // other servies (fopen, fclose, fread etc.) allowed for everyone
        return true;
    }
    // UI task
    if(dest == SRV_UI || dest == services[-SRV_UI]) {
        if(task_allowed(sender, "noui", 0)) goto noaccess;
        if(e==SYS_devprogress && sender->pid!=services[-SRV_FS]) goto noaccess;
        // other services allowed for everyone
        return true;
    }
    // syslog task
    if(dest == SRV_syslog || dest == services[-SRV_syslog]) {
        if(task_allowed(sender, "nosyslog", 0)) goto noaccess;
        // other services allowed for everyone
        return true;
    }
    // inet task
    if(dest == SRV_inet || dest == services[-SRV_inet]) {
        if(task_allowed(sender, "noinet", 0)) goto noaccess;
        // other services allowed for everyone
        return true;
    }
    // sound task
    if(dest == SRV_sound || dest == services[-SRV_sound]) {
        if(task_allowed(sender, "nosound", 0)) goto noaccess;
        // other services allowed for everyone
        return true;
    }
    // init task
    if(dest == SRV_init || dest == services[-SRV_init]) {
        if(task_allowed(sender, "noinit", 0)) goto noaccess;
        // other services allowed for everyone
        return true;
    }
    // video driver task
    if((dest == SRV_video || dest == services[-SRV_video]) && sender->pid != services[-SRV_UI]) goto noaccess;
    // if we made it this far, we are allowed to send the message
    return true;
noaccess:
    coreerrno = EPERM;
    return false;
}
