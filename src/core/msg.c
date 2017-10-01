/*
 * core/msg.c
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
 * @brief Message sender routines, called by isr_syscall in (platform)/isrc.S
 */

#include <arch.h>
#include <sysexits.h>
#include <sys/video.h>
#include <sys/mman.h>
#include "env.h"

/* external resources */
extern uint64_t env_getts(char *p,int16_t timezone);
extern uint64_t isr_currfps;
extern pid_t isr_next;
extern phy_t identity_mapping;
extern phy_t screen[2];
extern uint64_t coreerrno;

/* pointer to PDE for TMPQ_ADDRESS */
dataseg phy_t *mq_mapping;

/**
 * send a message with scalar values. Note msg_send() sends a buffer, it's a macro.
 */
uint64_t msg_sends(evt_t event, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5)
{
    tcb_t *srctcb = (tcb_t*)(0);
    tcb_t *dsttcb = (tcb_t*)(&tmpmap);
    msghdr_t *msghdr = (msghdr_t*)(TMPQ_ADDRESS + MQ_ADDRESS);
    uint64_t *paging = (uint64_t*)(&tmp2map);
    // don't use EVT_SENDER() here, would loose sign. This can be
    // negative pid if a service referenced.
    int64_t task = ((int64_t)(event&~0xFFFF)/(int64_t)65536);

    srand[event%4] *= srctcb->pid;
    kentropy();

    // only ISRs allowed to send SYS_IRQ message, and they are sent
    // on a more efficient way, directly with ksend
    if(EVT_FUNC(event)==0) {
        coreerrno = EPERM;
        return false;
    }
    // do we need pid_t translation?
    if(task < 0) {
        if(-task >= NUMSRV || services[-task]!=0) {
            task = services[-task];
        } else {
            coreerrno = ESRCH;
            return false;
        }
    }

    // security checks
    if(!msg_allowed(srctcb, task, event))
        return false;

    // map destination task's message queue
    kmap((uint64_t)&tmpmap, (uint64_t)((task!=0?task:srctcb->pid)*__PAGESIZE), PG_CORE_NOCACHE);
    if(dsttcb->magic != OSZ_TCB_MAGICH) {
        coreerrno = ESRCH;
        return false;
    }
    isr_next = dsttcb->memroot;
    kmap_mq(dsttcb->memroot);

    /* mappings:
     *  tmpmap: destination task's tcb
     *  tmp2map: destination task's message queue's PTE (paging)
     *  TMPQ_ADDRESS: destination task's self (message queue) */

    phy_t pte;
    size_t s;
    int bs = 0;
    void *p = (void *)(arg0 & ~(__PAGESIZE-1));
    /* TODO: use mq_buffstart properly as a circular buffer */
    // we have to save message buffer's address first
    if(event & MSG_PTRDATA){
        bs = (arg1+__PAGESIZE-1)/__PAGESIZE;
        // limit check
        if(msghdr->mq_buffstart/__PAGESIZE + bs >= nrmqmax + 1 + msghdr->mq_buffsize)
            msghdr->mq_buffstart = (nrmqmax + 1)*__PAGESIZE;
        // core memory cannot be sent in range -64M..0
        if((int64_t)p < 0 && (uint64_t)p > FBUF_ADDRESS) {
            coreerrno = EFAULT;
            return false;
        }
        // not mapping shared memory (not in the range -2^56..-64M).
        if((int64_t)p > 0) {
            // check size. Buffers over 1M must be sent in shared memory
            if(arg1 > (__SLOTSIZE/2)) {
                coreerrno = EINVAL;
                return false;
            }
            // modify pointer to point into the temporary buffer area
            arg0 = arg0 & (__PAGESIZE-1);
            arg0 += msghdr->mq_buffstart*__PAGESIZE;
            // get mapping for message buffer
            for(s = bs; s > 0; s --) {
                pte = *kmap_getpte((uint64_t)p);
                paging[msghdr->mq_buffstart] = (pte & ~(__PAGESIZE-1)) + PG_USER_RO;
                p += __PAGESIZE;
                msghdr->mq_buffstart++;
            }
        }
    }

#if DEBUG
    if(debug&DBG_MSG) {
        kprintf(" msg %2x -> %2x (%d), event #%x",
            srctcb->pid, task, msghdr->mq_start, EVT_FUNC(event));
        if(event & MSG_PTRDATA)
            kprintf("(*%x[%d]",arg0,arg1);
        else
            kprintf("(%x,%x",arg0,arg1);
        kprintf(",%x,%x,%x,%x)\n",arg2,arg3,arg4,arg5);
    }
#endif
    coreerrno = EBUSY;
    // send message to the mapped queue. Don't use EVT_FUNC, would
    // loose MSG_PTRDATA flag
    if(!ksend(msghdr,
        EVT_DEST((uint64_t)task) | (event&0xFFFF),
        arg0, arg1, arg2, arg3, arg4, arg5
    )) {
        if(srctcb->pid!=dsttcb->pid)
            sched_block(dsttcb);
        return false;
    } else {
        coreerrno = SUCCESS;
        if(srctcb->pid!=dsttcb->pid)
            sched_awake(dsttcb);
    }
    return true;
}

/**
 * System call dispatcher for messages sent to SRV_CORE. Also has a low level part,
 * isr_syscall in (platform)/isrc.S. We only came here if message is not handled there
 */
uint64_t msg_syscall(evt_t event, uint64_t arg0, uint64_t arg1, uint64_t arg2)
{
    tcb_t *tcb = (tcb_t*)0;
    phy_t *paging = (phy_t*)&tmpmap;
    phy_t *paging2 = (phy_t*)&tmp2map;
    void *data;
    char fn[128];
    phy_t tmp;
    uint64_t i,j;

    coreerrno = SUCCESS;
    switch(EVT_FUNC(event)) {
        /* case SYS_ack: in isr_syscall asm for performance */
        /* case SYS_sched_yield: in isr_syscall asm for performance */
        /* case SYS_rand: in isr_syscall asm for more bits */
        case SYS_exit:
            /* is it a critical service that's exiting? */
            if(tcb->pid == services[-SRV_FS] || tcb->pid == services[-SRV_UI]) {
                if(tcb->pid == services[-SRV_FS] && arg0==1)
                    kpanic("unable to mount root file system");
                kpanic("critical %s task exited",tcb->pid == services[-SRV_FS]?"FS":"UI");
            }
            /* is it init task exiting? */
            if(tcb->pid == services[-SRV_init]) {
                /* display good bye screen */
                if(arg0 == EX_OK)
                    kprintf_poweroff();
                else
                    kprintf_reboot();
            }
            /* is it an abort(), and core dump requested? */
            if(arg1) {
            }
            sched_pick();
            break;

        case SYS_dl:
            break;

        case SYS_swapbuf:
            /* only UI allowed to send swapbuf */
            if(msg_allowed(tcb, SRV_video, 0)) {
                isr_currfps++;
                /* swap screen[0] and screen[1] mapping */
                kmap((uint64_t)&tmpmap,  (uint64_t)(screen[0] & ~(__PAGESIZE-1)), PG_CORE_NOCACHE);
                i = (screen[0] & (__PAGESIZE-1))/sizeof(phy_t);
                kmap((uint64_t)&tmp2map, (uint64_t)(screen[1] & ~(__PAGESIZE-1)), PG_CORE_NOCACHE);
                j = (screen[1] & (__PAGESIZE-1))/sizeof(phy_t);
                tmp = paging[i]; paging[i] = paging2[j]; paging2[j] = tmp;
                /* flush screen buffer to video memory. */
                msg_sends(EVT_DEST(SRV_video) | EVT_FUNC(VID_flush), 0,0,0,0,0,0);
            }
            break;

        case SYS_stimebcd:
            /* set system time stamp in BCD date format (local time) */
            arg0 = env_getts((char*)&arg0, arg1);
            /* no break */
        case SYS_stime:
            /* set system time stamp in uint64_t (UTC) */
            if(msg_allowed(tcb, 0, SYS_stime)) {
                ticks[TICKS_TS] = arg0;
                ticks[TICKS_NTS] = 0;
            }
            break;

        case SYS_time:
            return ticks[TICKS_TS];

        case SYS_setirq:
            /* set irq handler, only device drivers allowed to do so */
            if(msg_allowed(tcb, 0, SYS_setirq)) {
                isr_installirq(arg0, tcb->memroot);
            }
            break;
    
        case SYS_alarm:
            /* suspend task for arg0 sec and arg1 microsec.
             * Values smaller than alarmstep already handled by isr_syscall in isrc.S */
            sched_alarm(tcb, ticks[TICKS_TS]+arg0, ticks[TICKS_NTS]+arg1);
            break;

        // FIXME: remove mapfile once vfs ready
        case SYS_mapfile:
            /* map a file info bss */
            if(arg0<BSS_ADDRESS || arg0>=BUF_ADDRESS || arg1==0 || *((char*)arg1)==0) {
                coreerrno = EINVAL;
                return 0;
            }
            /* locate the file */
            tmp = tcb->memroot;
            if(*((char*)arg1)=='/')
                arg1++;
            kstrcpy((char*)&fn,(char*)arg1);
            task_map(identity_mapping);
            data = fs_locate(fn);
            task_map(tmp);
            if(data==NULL) {
                coreerrno = ENOENT;
                return 0;
            }
            /* data inlined in inode? */
            if(data != NULL && ((phy_t)data & (__PAGESIZE-1))!=0) {
                /* copy to a new empty page */
                tmp = (phy_t)pmm_alloc(1);
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
                msg_sends(EVT_DEST(tcb->pid) | EVT_FUNC(SYS_ack), pmm.freepages,pmm.totalpages,0,0,0,0);
            break;

        case SYS_mmap:
#if DEBUG
        if(debug&DBG_MALLOC)
            kprintf("  mmap(%x, %d, %x) pid %2x\n", arg0, arg1, arg2, tcb->pid);
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
                    vmm_mapbss(tcb, (virt_t)arg0, (phy_t)pmm_alloc(1), __PAGESIZE, j);
                    tcb->allocmem++;
                    arg0+=__PAGESIZE;
                }
            }
            return (uint64_t)data;
        
        case SYS_munmap:
#if DEBUG
        if(debug&DBG_MALLOC)
            kprintf("  munmap(%x, %d) pid %2x\n", arg0, arg1, tcb->pid);
#endif
            arg1=((arg1+__PAGESIZE-1)/__PAGESIZE)*__PAGESIZE;
            vmm_unmapbss(tcb, (virt_t)arg0, (size_t)arg1);
            tcb->allocmem-=arg1/__PAGESIZE;
            break;

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
            coreerrno = EPERM;
            return (uint64_t)false;
    }
    return (uint64_t)true;
}
