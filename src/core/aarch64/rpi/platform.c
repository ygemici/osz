/*
 * core/aarch64/rpi/platform.c
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
 * @brief Platform glue code
 */

#include "../arch.h"
#include "platform.h"

extern void armtmr_init();
extern void systmr_init();

extern phy_t identity_mapping;
extern uint8_t dbg_iskbd;
extern uint64_t lastpt;
extern unsigned char *env_hex(unsigned char *s, uint64_t *v, uint64_t min, uint64_t max);
extern unsigned char *env_dec(unsigned char *s, uint64_t *v, uint64_t min, uint64_t max);
volatile uint32_t  __attribute__((aligned(16))) mbox[16];
extern int scry;

/* mailbox functions */
uint8_t mbox_call(uint8_t ch, volatile uint32_t *mbox)
{
    uint32_t r;
    do{asm volatile("nop");}while(*MBOX_STATUS & MBOX_FULL);
    *MBOX_WRITE = (((uint32_t)((uint64_t)mbox)&~0xF) | (ch&0xF));
    while(1) {
        do{asm volatile("nop");}while(*MBOX_STATUS & MBOX_EMPTY);
        r=*MBOX_READ;
        if((uint8_t)(r&0xF)==ch)
            return (r&~0xF)==(uint32_t)((uint64_t)mbox) && mbox[1]==MBOX_RESPONSE;
    }
    return false;
}

/**
 * parse clocksource configuration
 */
unsigned char *env_cs(unsigned char *s)
{
    uint64_t tmp;
    if(*s>='0' && *s<='9') {
        s = env_dec(s, &tmp, 0, 3);
        clocksource=(uint8_t)tmp;
        return s;
    }
    // skip separators
    while(*s==' '||*s=='\t')
        s++;
    clocksource=0;
    if(s[0]=='a' && s[1]=='r')  clocksource=TMR_ARM;  // CPU timer
    if(s[0]=='s' && s[1]=='y')  clocksource=TMR_SYS;  // system timer
    while(*s!=0 && *s!='\n')
        s++;
    return s;
}

/**
 * Initalize platform variables. Called by env_init()
 */
void platform_env()
{
    //drain AUX buffer
    register uint64_t r;
    while((*AUX_MU_LSR&0x01)) r=(uint64_t)(*AUX_MU_IO);
    r++; //make gcc happy
    scry=-1;
}

/**
 * Parse platform specific parameters, called by env_init()
 */
unsigned char *platform_parse(unsigned char *env)
{
    // manually override clock source
    if(!kmemcmp(env, "clock=", 6)) {
        env += 6;
        env = env_cs(env);
    } else
        env++;
    return env;
}

/**
 * Set up timer. Called by isr_init()
 */
void platform_timer()
{
    // no autodetection, ARM CPU timer always there
    if(clocksource==0)
        clocksource=TMR_ARM;

    if(clocksource==TMR_ARM)
        armtmr_init();
    else
        systmr_init();
}

/**
 * Initalize platform, detect basic hardware. Called by sys_init()
 */
void platform_detect()
{
    // things needed for isr_init()
    numcores=4;
}

/**
 * Detect devices on platform. Called by sys_init()
 */
void platform_enumerate()
{
}

/**
 * map driver memory into task's address space. Called by drv_init()
 */
virt_t platform_mapdrvmem(tcb_t *tcb)
{
    virt_t v=(((lastpt+511)/512)*512)*__PAGESIZE;
    vmm_mapbss(tcb, v, *((phy_t*)vmm_getpte(MMIO_BASE))&~((0xFFFFUL<<48)|0xFFF), 64*1024*1024, PG_USER_DRVMEM); 
    return v;
}


/**
 * Power off the platform. Called by kprintf_poweroff()
 */
void platform_poweroff()
{
    uint64_t r=10000;
    // flush AUX
    do{asm volatile("nop");}while(r-- && ((*AUX_MU_LSR&0x20)||(*AUX_MU_LSR&0x01)));r=*AUX_MU_IO;
    asm volatile("dsb sy; isb");
    // disable MMU cache
    vmm_switch(identity_mapping);
    asm volatile ("mrs %0, sctlr_el1" : "=r" (r));
    r&=~((1<<12) |   // clear I, no instruction cache
         (1<<2));    // clear C, no cache at all
    asm volatile ("msr sctlr_el1, %0; isb" : : "r" (r));
    // power off devices one by one
    for(r=0;r<16;r++) {
        mbox[0]=8*4;
        mbox[1]=MBOX_REQUEST;
        mbox[2]=0x28001;        // set power state
        mbox[3]=8;
        mbox[4]=8;
        mbox[5]=(uint32_t)r;    // device id
        mbox[6]=0;              // bit 0: off, bit 1: no wait
        mbox[7]=0;
        mbox_call(MBOX_CH_PROP,mbox);
    }
    // power off gpio pins
    *GPFSEL0 = 0; *GPFSEL1 = 0; *GPFSEL2 = 0; *GPFSEL3 = 0; *GPFSEL4 = 0; *GPFSEL5 = 0;
    *GPPUD = 0;
    for(r=150;r>0;r--) { asm volatile("nop"); }
    *GPPUDCLK0 = 0xffffffff; *GPPUDCLK1 = 0xffffffff;
    for(r=150;r>0;r--) { asm volatile("nop"); }
    *GPPUDCLK0 = 0; *GPPUDCLK1 = 0;        // flush GPIO setup
    // power off the SoC (GPU + CPU)
    r = *PM_RSTS; r &= ~0xfffffaaa;
    r |= 0x555;    // partition 63 used to indicate halt
    *PM_RSTS = PM_WDOG_MAGIC | r;
    *PM_WDOG = PM_WDOG_MAGIC | 10;
    *PM_RSTC = PM_WDOG_MAGIC | PM_RSTC_FULLRST;
}

/**
 * Reboot computer. Called by kprintf_reboot()
 */
void platform_reset()
{
    uint32_t r=10000;
    // flush AUX
    do{asm volatile("nop");}while(r-- && ((*AUX_MU_LSR&0x20)||(*AUX_MU_LSR&0x01)));r=*AUX_MU_IO;
    asm volatile("dsb sy; isb");
    r = *PM_RSTS; r &= ~0xfffffaaa;
    *PM_RSTS = PM_WDOG_MAGIC | r;   // partition 0
    *PM_WDOG = PM_WDOG_MAGIC | 10;
    *PM_RSTC = PM_WDOG_MAGIC | PM_RSTC_FULLRST;
}

/**
 * Hang computer.
 */
void platform_halt()
{
    asm volatile("1: wfe; b 1b");
}

/**
 * Initialize random seed
 */
void platform_srand()
{
    register uint64_t i;
    *RNG_STATUS=0x40000; *RNG_INT_MASK|=1; *RNG_CTRL|=1;
    while(!((*RNG_STATUS)>>24)) asm volatile("nop");
    for(i=0;i<4;i++) {
        srand[i]+=*RNG_DATA;
        srand[i]^=(uint64_t)(*RNG_DATA)<<16;
    }
}

/**
 * an early implementation, called by kprintf
 */
uint64_t platform_waitkey()
{
    register uint64_t r;
    do{asm volatile("nop");}while(!(*AUX_MU_LSR&0x01));r=(uint64_t)(*AUX_MU_IO);
    dbg_iskbd=false;
    return r==13?10:r;
}

#if DEBUG
/**
 * initialize debug console
 */
void platform_dbginit()
{
    uint32_t r;
    /* initialize UART */
    *UART0_CR = 0;         // turn off UART0 or real hw. qemu doesn't care
    *AUX_ENABLE |=1;       // enable UART1, mini uart
    *AUX_MU_IER = 0;
    *AUX_MU_CNTL = 0;
    *AUX_MU_LCR = 3;       // 8 bits
    *AUX_MU_MCR = 0;
    *AUX_MU_IER = 0;
    *AUX_MU_IIR = 0xc6;    // disable interrupts
    *AUX_MU_BAUD = 270;    // 115200 baud
    r=*GPFSEL1;
    r&=~((7<<12)|(7<<15)); // gpio14, gpio15
    r|=(2<<12)|(2<<15);    // alt5
    *GPFSEL1 = r;
    *GPPUD = 0;
    for(r=150;r>0;r--) { asm volatile("nop"); }
    *GPPUDCLK0 = (1<<14)|(1<<15);
    for(r=150;r>0;r--) { asm volatile("nop"); }
    *GPPUDCLK0 = 0;        // flush GPIO setup
    *AUX_MU_CNTL = 3;      // enable Tx, Rx
}

/**
 * display a character on debug console
 */
void platform_dbgputc(uint8_t c)
{
    do{asm volatile("nop");}while(!(*AUX_MU_LSR&0x20)); *AUX_MU_IO=c; *UART0_DR=c;
}
#endif
