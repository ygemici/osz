/*
 * core/x86_64/isr.c
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
 * @brief Interrupt Service Routines
 */


#include "platform.h"
#include "isr.h"
#include "../env.h"
#include <fsZ.h>

/* external resources */
extern OSZ_ccb ccb;                   // CPU Control Block

/* from isrs.S */
extern void isr_exc00divzero();
extern void isr_irq0();
extern void isr_inithw(uint64_t *idt, OSZ_ccb *tss);

extern uint64_t core_mapping;
extern uint64_t ioapic_addr;
extern uint64_t freq;
extern uint64_t fpsdiv;
extern uint64_t quantumdiv;
extern sysinfo_t *sysinfostruc;

#if DEBUG
extern void dbg_enable(uint64_t rip);
#endif

/* counters and timestamps */
uint64_t __attribute__ ((section (".data"))) isr_ticks[8];
/* 256 bit random seed */
uint64_t __attribute__ ((section (".data"))) isr_entropy[4];
/* current fps counter and last sec value */
uint64_t __attribute__ ((section (".data"))) isr_currfps;
uint64_t __attribute__ ((section (".data"))) isr_lastfps;
uint64_t __attribute__ ((section (".data"))) isr_next;

/* System call dispatcher for messages sent to SRV_core */
bool_t isr_syscall(pid_t thread, uint64_t event, void *ptr, size_t size, uint64_t magic)
{
    OSZ_tcb *tcb = (OSZ_tcb*)0;
    switch(MSG_FUNC(event)) {
        /* case SYS_ack: in isr_syscall0 asm for performance */
        /* case SYS_seterr: in isr_syscall0 asm for performance */
        /* case SYS_sched_yield: in isr_syscall0 asm for performance */
        case SYS_exit:
            break;
        case SYS_sysinfo:
            // dynamic fields in System Info Block
            sysinfostruc->ticks[0] = isr_ticks[TICKS_LO];
            sysinfostruc->ticks[1] = isr_ticks[TICKS_HI];
            sysinfostruc->timestamp_s = isr_ticks[TICKS_TS];
            sysinfostruc->timestamp_ns = isr_ticks[TICKS_NTS];
            sysinfostruc->srand[0] = isr_entropy[0];
            sysinfostruc->srand[1] = isr_entropy[1];
            sysinfostruc->srand[2] = isr_entropy[2];
            sysinfostruc->srand[3] = isr_entropy[3];
            sysinfostruc->fps = isr_lastfps;
            msg_send(tcb->mypid, MSG_PTRDATA | MSG_FUNC(SYS_ack),
                (void*)sysinfostruc,
                sizeof(sysinfo_t),
                SYS_sysinfo);
            // when SYS task signals a boot eoi, enable interrupts
            if(tcb->memroot == sys_mapping && (uint64_t)ptr==0xB0070E01) {
                tcb->rflags |= 0x200;
            }
            break;
        case SYS_swapbuf:
            isr_currfps++;
            /* TODO: map and swap screen[0] and screen[1] */
            /* flush screen buffer to video memory */
            msg_sends(services[-SRV_SYS], MSG_FUNC(SYS_swapbuf),0,0,0,0,0,0);
            break;
        case SYS_stime:
            /* set system time stamp (UTC) */
            if(tcb->memroot == sys_mapping || thread_allowed("stime",FSZ_WRITE)) {
                isr_ticks[TICKS_TS] = (uint64_t)ptr;
            }
            break;
        default:
            tcb->errno = EINVAL;
            return false;
    }
    return true;
}

/* Initialize interrupts */
void isr_init()
{
    uint64_t *idt = kalloc(1);      //allocate Interrupt Descriptor Table
    void *ptr;
    int i;

    isr_next = 0;

    // generate IDT
    ptr = &isr_exc00divzero;
    // 0-31 exception handlers
    for(i=0;i<32;i++) {
        idt[i*2+0] = IDT_GATE_LO(i==2||i==8?IDT_NMI:IDT_EXC, ptr);
        idt[i*2+1] = IDT_GATE_HI(ptr);
        ptr+=ISR_EXCMAX;
    }
    // 32-255 irq handlers
    ptr = &isr_irq0;
    for(i=32;i<ISR_NUMIRQ+32;i++) {
        idt[i*2+0] = IDT_GATE_LO(IDT_INT, ptr);
        idt[i*2+1] = IDT_GATE_HI(ptr);
        ptr+=ISR_IRQMAX;
    }
    // set up isr_syscall dispatcher and IDTR, also mask all IRQs
    isr_inithw(idt, &ccb);
}

/* fallback exception handler */
void excabort(uint64_t excno, uint64_t rip, uint64_t errcode)
{
    kpanic("---- exception %d ----",excno);
}

/* exception specific code */
void exc00divzero(uint64_t excno, uint64_t rip)
{
    kpanic("divzero %d",excno);
}

void exc01debug(uint64_t excno, uint64_t rip)
{
    kpanic("debug %d",excno);
}

void exc03chkpoint(uint64_t excno, uint64_t rip)
{
#if DEBUG
    dbg_enable(rip);
#endif
}

void exc13genprot(uint64_t excno, uint64_t rip)
{
    kpanic("General Protection Fault %d",excno);
}
