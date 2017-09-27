/*
 * core/security.c
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
 * @brief Security functions
 */

#include <arch.h>

/* errno in core */
dataseg uint64_t coreerrno;

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
        if(g[0]==0)
            return false;
        if(!kmemcmp(g, grp, j))
            return (g[15] & access)!=0;
    }
    return false;
}

/**
 * check if current task has can send the message
 */
bool_t msg_allowed(tcb_t *sender, pid_t dest, evt_t event)
{
    /* IRQ's SYS_ack only allowed for drivers, inlined check in isr_syscall */
    if(dest == services[-SRV_FS]) {
        // only init allowed to send SYS_mountfs to FS
        if(event==SYS_mountfs && sender->pid!=services[-SRV_init]) goto noaccess;
        // only drivers and system services allowed to send SYS_mknod to FS
        if(event==SYS_mknod && sender->priority!=PRI_DRV && sender->priority!=PRI_SRV) goto noaccess;
        return true;
    }
    if(dest == SRV_video && sender->pid != services[-SRV_UI]) goto noaccess;
    if(event == SYS_stime && !(sender->priority == PRI_DRV || task_allowed(sender, "stime", A_WRITE))) goto noaccess;
    if(event == SYS_setirq && sender->priority != PRI_DRV) goto noaccess;
    return true;
noaccess:
    coreerrno = EPERM;
    return false;
}
