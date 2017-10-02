/*
 * core/x86_64/acpi/platform.c
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

extern uint32_t SLP_EN;
extern uint32_t SLP_TYPa;
extern uint32_t PM1a_CNT;
extern uint32_t SLP_TYPb;
extern uint32_t PM1b_CNT;
extern uint32_t PM1_CNT_LEN;
extern void acpi_init();
extern void acpi_early(void *hdr);
extern void pit_init();
extern void hpet_init();

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
    if(s[0]=='h' && s[1]=='p')  clocksource=TMR_HPET; // HPET
    if(s[0]=='p' && s[1]=='i')  clocksource=TMR_PIT;  // PIT
    while(*s!=0 && *s!='\n')
        s++;
    return s;
}

/**
 * Initialize platform dependent part. Called by main()
 */
void platform_init()
{
    // set up defaults
    systables[systable_acpi_idx] = bootboot.x86_64.acpi_ptr;
    systables[systable_smbi_idx] = bootboot.x86_64.smbi_ptr;
    systables[systable_efi_idx] = bootboot.x86_64.efi_ptr;
    systables[systable_mp_idx] = bootboot.x86_64.mp_ptr;
    systables[systable_apic_idx] = systables[systable_ioapic_idx] = 
    systables[systable_hpet_idx] = 0;
    systables[systable_dsdt_idx] = (uint64_t)fs_locate("sys/etc/dsdt");
    if(fs_size == 0)
        systables[systable_dsdt_idx] = 0;
    numcores=1;
    acpi_early(NULL);
}

/**
 * Parse platform specific parameters, called by env_init()
 */
unsigned char *platform_parse(unsigned char *env)
{
    // manually override HPET address
    if(!kmemcmp(env, "hpet=", 5)) {
        env += 5;
        // we only accept hex value for this parameter
        env = env_hex(env, (uint64_t*)&systables[systable_hpet_idx], 1024*1024, 0);
        clocksource = TMR_HPET;
    } else
    // manually override APIC address
    if(!kmemcmp(env, "apic=", 5)) {
        env += 5;
        // we only accept hex value for this parameter
        env = env_hex(env, (uint64_t*)&systables[systable_apic_idx], 1024*1024, 0);
    } else
    // manually override IOAPIC address
    if(!kmemcmp(env, "ioapic=", 7)) {
        env += 7;
        // we only accept hex value for this parameter
        env = env_hex(env, (uint64_t*)&systables[systable_ioapic_idx], 1024*1024, 0);
    } else
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
    if (systables[systable_hpet_idx]==0 || clocksource==TMR_PIT) {
        // fallback to PIT if HPET not found
        pit_init();
        clocksource=TMR_PIT;
    } else {
        hpet_init();
        clocksource=TMR_HPET;
    }
}

/**
 * Detect devices on platform. Called by sys_init()
 */
void platform_enumerate()
{
    acpi_init();
}

/**
 * Power off the platform. Called by kprintf_poweroff()
 */
void platform_poweroff()
{
    uint64_t en;
    if(PM1a_CNT!=0) {
        en = SLP_TYPa | SLP_EN;
        __asm__ __volatile__ (
            "movq %0, %%rax; movl %1, %%edx; outw %%ax, %%dx" : :
            "a"(en), "d"(PM1a_CNT) );
        if ( PM1b_CNT != 0 ) {
            en = SLP_TYPb | SLP_EN;
            __asm__ __volatile__ (
                "movq %0, %%rax; movl %1, %%edx; outw %%ax, %%dx" : :
                "a"(en), "d"(PM1b_CNT) );
        }
    }
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
