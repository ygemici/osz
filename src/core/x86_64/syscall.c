/*
 * core/x86_64/syscall.c
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
 * @brief Syscall dispatcher
 */


#include "platform.h"
#include "isr.h"
#include "../env.h"
#include <fsZ.h>

/* external resources */
extern OSZ_ccb ccb;                   // CPU Control Block

extern uint64_t sys_getts(char *p);
extern sysinfo_t *sysinfostruc;
extern uint64_t isr_ticks[8];
extern uint64_t isr_entropy[4];
extern uint64_t isr_lastfps;
extern uint64_t isr_currfps;

/* System call dispatcher for messages sent to SRV_core */
bool_t isr_syscall(evt_t event, void *ptr, size_t size, uint64_t magic)
{
    OSZ_tcb *tcb = (OSZ_tcb*)0;
    switch(EVT_FUNC(event)) {
        /* case SYS_ack: in isr_syscall0 asm for performance */
        /* case SYS_seterr: in isr_syscall0 asm for performance */
        case SYS_exit:
            break;

        case SYS_sysinfo:
            // dynamic fields in System Info Block
            sysinfostruc->ticks[0] = isr_ticks[TICKS_LO];
            sysinfostruc->ticks[1] = isr_ticks[TICKS_HI];
            sysinfostruc->quantumcnt = isr_ticks[TICKS_QALL];
            sysinfostruc->timestamp_s = isr_ticks[TICKS_TS];
            sysinfostruc->timestamp_ns = isr_ticks[TICKS_NTS];
            sysinfostruc->srand[0] = isr_entropy[0];
            sysinfostruc->srand[1] = isr_entropy[1];
            sysinfostruc->srand[2] = isr_entropy[2];
            sysinfostruc->srand[3] = isr_entropy[3];
            sysinfostruc->fps = isr_lastfps;
            msg_send(EVT_DEST(tcb->mypid) | MSG_PTRDATA | EVT_FUNC(SYS_ack),
                (void*)sysinfostruc,
                sizeof(sysinfo_t),
                SYS_sysinfo);
            break;

        case SYS_swapbuf:
            isr_currfps++;
            /* TODO: map and swap screen[0] and screen[1] */
            /* flush screen buffer to video memory */
            msg_sends(EVT_DEST(SRV_CORE) | EVT_FUNC(SYS_swapbuf), 0,0,0,0,0,0);
            break;

        case SYS_stime:
            /* set system time stamp (UTC) */
            if(tcb->memroot == sys_mapping || thread_allowed("stime",FSZ_WRITE)) {
                isr_ticks[TICKS_TS] = sys_getts((char*)&ptr);
                isr_ticks[TICKS_NTS] = 0;
            }
            break;

        default:
            tcb->errno = EINVAL;
            return false;
    }
    return true;
}
