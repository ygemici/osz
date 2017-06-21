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
#include <sysexits.h>
#include <sys/video.h>
#include <sys/mman.h>
#include "env.h"

/* external resources */
extern OSZ_ccb ccb;                   // CPU Control Block

extern uint64_t isr_getts(char *p,int16_t timezone);
extern uint64_t isr_currfps;
extern phy_t identity_mapping;
extern pid_t identity_pid;
extern phy_t screen[2];

/**
 * System call dispatcher for messages sent to SRV_CORE. Also has a low level
 * part, isr_syscall0 in isrc.S. We only came here if message is not handled there
 */
uint64_t isr_syscall(evt_t event, uint64_t arg0, uint64_t arg1, uint64_t arg2)
{
    OSZ_tcb *tcb = (OSZ_tcb*)0;
    phy_t *paging = (phy_t*)&tmpmap;
    phy_t *paging2 = (phy_t*)&tmp2map;
    void *data;
    char fn[128];
    phy_t tmp;
    uint64_t i,j;

    tcb->errno = SUCCESS;
    switch(EVT_FUNC(event)) {
        /* case SYS_ack: in isr_syscall0 asm for performance */
        /* case SYS_seterr: in isr_syscall0 asm for performance */
        case SYS_exit:
            if(identity_pid && tcb->mypid == identity_pid) {
                /* notify init service to start */
                msg_sends(EVT_DEST(SRV_init) | EVT_FUNC(SYS_ack),0,0,0,0,0,0);
            }
            if(tcb->mypid == services[-SRV_init]) {
                /* power off or reboot system when init task exits */
                if(arg0 == EX_OK)
                    sys_disable();
                else
                    sys_reset();
            }
            sched_pick();
            break;

        case SYS_dl:
            break;

        case SYS_swapbuf:
            /* only UI allowed to send swapbuf */
            if(tcb->mypid == services[-SRV_UI]) {
                isr_currfps++;
                /* swap screen[0] and screen[1] mapping */
                kmap((uint64_t)&tmpmap,  (uint64_t)(screen[0] & ~(__PAGESIZE-1)), PG_CORE_NOCACHE);
                i = (screen[0] & (__PAGESIZE-1))/sizeof(phy_t);
                kmap((uint64_t)&tmp2map, (uint64_t)(screen[1] & ~(__PAGESIZE-1)), PG_CORE_NOCACHE);
                j = (screen[1] & (__PAGESIZE-1))/sizeof(phy_t);
                tmp = paging[i]; paging[i] = paging2[j]; paging2[j] = tmp;
                /* flush screen buffer to video memory. */
                msg_sends(EVT_DEST(SRV_video) | EVT_FUNC(VID_flush), 0,0,0,0,0,0);
            } else
                tcb->errno = EACCES;
            break;

        case SYS_stimebcd:
            /* set system time stamp in BCD date format (local time) */
            arg0 = isr_getts((char*)&arg0, arg1);
            /* no break */
        case SYS_stime:
            /* set system time stamp in uint64_t (UTC) */
            if(tcb->priority == PRI_DRV || thread_allowed("stime", A_WRITE)) {
                ticks[TICKS_TS] = arg0;
                ticks[TICKS_NTS] = 0;
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

        case SYS_setirq:
            /* set irq handler, only device drivers allowed to do so */
            if(tcb->priority == PRI_DRV) {
                isr_installirq(arg0, tcb->memroot);
            } else {
                tcb->errno = EACCES;
            }
            break;
    
        case SYS_alarm:
            /* suspend thread for arg0 sec and arg1 microsec.
             * Values smaller than alarmstep already handled by isr_syscall0 in isrc.S */
            sched_alarm(tcb, ticks[TICKS_TS]+arg0, ticks[TICKS_NTS]+arg1);
            break;

        case SYS_mapfile:
            /* map a file info bss */
            if(arg0<BSS_ADDRESS || arg0>=BUF_ADDRESS || arg1==0 || *((char*)arg1)==0) {
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

        case SYS_meminfo:
                msg_sends(EVT_DEST(tcb->mypid) | EVT_FUNC(SYS_ack), pmm.freepages,pmm.totalpages,0,0,0,0);
            break;

        case SYS_mmap:
#if DEBUG
        if(debug&DBG_MALLOC)
            kprintf("  mmap(%x, %d, %x) pid %2x\n", arg0, arg1, arg2, tcb->mypid);
#endif
            /* FIXME: naive implementation, no checks */
            data=(void *)arg0;
            j=arg2&PROT_WRITE? PG_USER_RW : PG_USER_RO;
            if(arg1>=__SLOTSIZE) {
                arg1=(arg1+__SLOTSIZE-1)/__SLOTSIZE;
                for(i=0;i<arg1;i++) {
                    vmm_mapbss(tcb, (virt_t)arg0, (phy_t)pmm_allocslot(), __SLOTSIZE, j);
                    tcb->allocmem+=__SLOTSIZE/__PAGESIZE;
                    arg0+=__SLOTSIZE;
                }
            } else {
                arg1=(arg1+__PAGESIZE-1)/__PAGESIZE;
                for(i=0;i<arg1;i++) {
                    vmm_mapbss(tcb, (virt_t)arg0, (phy_t)pmm_alloc(), __PAGESIZE, j);
                    tcb->allocmem++;
                    arg0+=__PAGESIZE;
                }
            }
            return (uint64_t)data;
        
        case SYS_munmap:
#if DEBUG
        if(debug&DBG_MALLOC)
            kprintf("  munmap(%x, %d) pid %2x\n", arg0, arg1, tcb->mypid);
#endif
            arg1=((arg1+__PAGESIZE-1)/__PAGESIZE)*__PAGESIZE;
            vmm_unmapbss(tcb, (virt_t)arg0, (size_t)arg1);
            tcb->allocmem-=arg1/__PAGESIZE;
            break;

        case SYS_rand:
            return srand[0] ^ srand[1] ^ srand[2] ^ srand[3];
        
        case SYS_srand:
            srand[(arg0+0)%4] += (uint64_t)arg0;
            srand[(arg0+1)%4] ^= (uint64_t)arg0;
            srand[(arg0+2)%4] += (uint64_t)arg0;
            srand[(arg0+3)%4] ^= (uint64_t)arg0;
            srand[(arg0+0)%4] *= 16807;
            srand[(arg0+1)%4] *= ticks[TICKS_LO];
            kentropy();
            break;

        default:
            tcb->errno = EPERM;
            return (uint64_t)false;
    }
    return (uint64_t)true;
}
