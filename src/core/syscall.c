/*
 * core/syscall.c
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
 * @brief Platform independent syscall dispatcher. Called by isr_syscall0
 */


#include <fsZ.h>
#include <sys/sysinfo.h>
#include "env.h"

/* external resources */
extern OSZ_ccb ccb;                   // CPU Control Block

extern uint64_t isr_getts(char *p,int16_t timezone);
extern uint64_t isr_currfps;
extern phy_t identity_mapping;
extern sysinfo_t sysinfostruc;

/* System call dispatcher for messages sent to SRV_core */
uint64_t isr_syscall(evt_t event, uint64_t arg0, uint64_t arg1, uint64_t arg2)
{
    OSZ_tcb *tcb = (OSZ_tcb*)0;
    void *data;
    char fn[128];
    phy_t tmp;

    tcb->errno = SUCCESS;
    switch(EVT_FUNC(event)) {
        /* case SYS_ack: in isr_syscall0 asm for performance */
        /* case SYS_seterr: in isr_syscall0 asm for performance */
        case SYS_exit:
            break;

        case SYS_dl:
            break;

        case SYS_sysinfo:
            // update dynamic fields
            sysinfostruc.mem_free = pmm.freepages;
            // reply with ptr message to map sysinfostruc in userspace
            msg_send(EVT_DEST(tcb->mypid) | MSG_PTRDATA | EVT_FUNC(SYS_ack),
                (virt_t)&sysinfostruc,
                sizeof(sysinfo_t),
                SYS_sysinfo);
            break;

        case SYS_swapbuf:
            isr_currfps++;
            /* TODO: swap and map screen[0] and screen[1] */

            /* flush screen buffer to video memory. Note this is a different
             * call than we're serving. We came here because
             *  SRV_UI sent SYS_swapbuf to SRV_CORE
             * And now
             *  SRV_CORE sends SYS_swapbuf to SRV_SYS */
            //msg_sends(EVT_DEST(SRV_SYS) | EVT_FUNC(SYS_swapbuf), 0,0,0,0,0,0);
            break;

        case SYS_stimebcd:
            /* set system time stamp in BCD date format (local time) */
            arg0 = isr_getts((char*)&arg0, arg1);
            /* no break */
        case SYS_stime:
            /* set system time stamp in uint64_t (UTC) */
            if(tcb->priority == PRI_DRV || thread_allowed("stime", A_WRITE)) {
                sysinfostruc.ticks[TICKS_TS] = arg0;
                sysinfostruc.ticks[TICKS_NTS] = 0;
            } else {
                tcb->errno = EACCES;
            }
            break;

        case SYS_regservice:
            /* only init subsystem allowed to register services */
            if(tcb->mypid == services[-SRV_init])
                return (uint64_t)service_register((pid_t)arg0);
            tcb->errno = EACCES;
            break;

        case SYS_mapfile:
            /* map a file info bss */
            if(arg0<BSS_ADDRESS || arg0>=FBUF_ADDRESS || arg1==0 || *((char*)arg1)==0) {
                tcb->errno = EINVAL;
                return 0;
            }
            /* locate the file */
            tmp = tcb->memroot;
            if(*((char*)arg1)=='/')
                arg1++;
            kstrcpy((char*)&fn,(char*)arg1);
            thread_map(identity_mapping);
            data = fs_locate(fn);
            thread_map(tmp);
            if(data==NULL) {
                tcb->errno = ENOENT;
                return 0;
            }
            /* data inlined in inode? */
            if(data != NULL && ((phy_t)data & (__PAGESIZE-1))!=0) {
                /* copy to a new empty page */
                tmp = (phy_t)pmm_alloc();
                tcb->allocmem++;
                tcb->linkmem--;
                kmap((virt_t)&tmpmap, (phy_t)data & ~(__PAGESIZE-1), PG_CORE_NOCACHE);
                kmap((virt_t)&tmp2map, tmp, PG_CORE_NOCACHE);
                kmemcpy((char*)&tmp2map, &tmpmap + ((phy_t)data & (__PAGESIZE-1)), fs_size);
                data = (void*)tmp;
            }
            /* map */
            vmm_mapbss(tcb, (virt_t)arg0, (phy_t)data, fs_size, PG_USER_RO);
            /* return loaded file size */
            return fs_size;

        default:
            tcb->errno = ENXIO;
            return (uint64_t)false;
    }
    return (uint64_t)true;
}
