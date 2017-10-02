/*
 * core/x86_64/ibmpc/platform.c
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
#include "../isr.h"

extern void pci_init();
extern void pit_init();
extern void rtc_init();

extern unsigned char *env_hex(unsigned char *s, uint64_t *v, uint64_t min, uint64_t max);
extern unsigned char *env_dec(unsigned char *s, uint64_t *v, uint64_t min, uint64_t max);

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
    if(s[0]=='p' && s[1]=='i')  clocksource=TMR_PIT;  // PIT
    if(s[0]=='r' && s[1]=='t')  clocksource=TMR_RTC;  // RTC
    while(*s!=0 && *s!='\n')
        s++;
    return s;
}

/**
 * Initalize platform. Called by main()
 */
void platform_init()
{
    // set up defaults
    systables[systable_smbi_idx] = bootboot.x86_64.smbi_ptr;
    systables[systable_mp_idx] = bootboot.x86_64.mp_ptr;

    systables[systable_acpi_idx] = systables[systable_efi_idx] =
    systables[systable_apic_idx] = systables[systable_ioapic_idx] = 
    systables[systable_hpet_idx] = systables[systable_dsdt_idx] = 0;

    clocksource=TMR_PIT;

    numcores=1;
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
    if(clocksource==TMR_RTC) {
        rtc_init();
    } else {
        clocksource=TMR_PIT;
        pit_init();
    }
}

/**
 * Detect devices on platform. Called by sys_init()
 */
void platform_enumerate()
{
// TODO: enumerate bus and load drivers
    pci_init();
}

/**
 * Power off the platform. Called by kprintf_poweroff()
 */
void platform_poweroff()
{
    // Bochs poweroff hack
    char *s = "Shutdown";
    while (*s) {
        __asm__ __volatile__ ("movw $0x8900, %%dx; outb %0, %%dx" : : "a"(*s));
        s++;
    }
    // implement APM poweroff maybe?
    // Not sure about BIOS, we maybe booted on UEFI
}

/**
 * Reboot computer. Called by kprintf_reboot()
 */
void platform_reset()
{
    __asm__ __volatile__ ("movb $0xFE, %al; outb %al, $0x64");
}

/**
 * Hang computer.
 */
void platform_halt()
{
    __asm__ __volatile__ ("1: cli; hlt; jmp 1b");
}
