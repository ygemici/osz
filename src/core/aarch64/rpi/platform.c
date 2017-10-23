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

extern unsigned char *env_hex(unsigned char *s, uint64_t *v, uint64_t min, uint64_t max);
extern unsigned char *env_dec(unsigned char *s, uint64_t *v, uint64_t min, uint64_t max);

/**
 * Initalize platform variables. Called by env_init()
 */
void platform_env()
{
}

/**
 * Parse platform specific parameters, called by env_init()
 */
unsigned char *platform_parse(unsigned char *env)
{
    return env+1;
}

/**
 * Set up timer. Called by isr_init()
 */
void platform_timer()
{
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
 * Power off the platform. Called by kprintf_poweroff()
 */
void platform_poweroff()
{
}

/**
 * Reboot computer. Called by kprintf_reboot()
 */
void platform_reset()
{
    uint32_t cnt=10000;
    // flush AUX
    do{asm volatile("nop");}while(cnt-- && ((*AUX_MU_LSR&0x20)||(*AUX_MU_LSR&0x01)));cnt=*AUX_MU_IO;
    *PM_WATCHDOG = PM_WDOG_MAGIC | 1;
    *PM_RTSC = PM_WDOG_MAGIC | PM_RTSC_FULLRST;
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
    uint64_t i=10000;
    *RNG_STATUS=0x40000; *RNG_INT_MASK|=1; *RNG_CTRL|=1;
    do{asm volatile("nop");}while(i--); while(!((*RNG_STATUS)>>24)) asm volatile("nop");
    for(i=0;i<4;i++)
        srand[i]^=*RNG_DATA;
}

/**
 * an early implementation, called by kprintf
 */
uint64_t platform_waitkey()
{
    uint64_t r;do{asm volatile("nop");}while(!(*AUX_MU_LSR&0x01));r=(uint64_t)(*AUX_MU_IO);return r==13?10:r;
}

#if DEBUG
/**
 * initialize debug console
 */
void platform_dbginit()
{
}

/**
 * display a character on debug console
 */
void platform_dbgputc(int c)
{
    do{asm volatile("nop");}while(!(*AUX_MU_LSR&0x20)); *AUX_MU_IO=c; *UART0_DR=c;
}
#endif
