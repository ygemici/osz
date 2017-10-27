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
 * Initalize platform variables. Called by env_init()
 */
void platform_env()
{
    // set up defaults. Remember, no PMM yet
    systables[systable_smbi_idx] = bootboot.x86_64.smbi_ptr;
    systables[systable_mp_idx] = bootboot.x86_64.mp_ptr;

    systables[systable_acpi_idx] = systables[systable_efi_idx] =
    systables[systable_apic_idx] = systables[systable_ioapic_idx] = 
    systables[systable_hpet_idx] = systables[systable_dsdt_idx] = 0;

    clocksource=TMR_PIT;
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
 * Initalize platform, detect basic hardware. Called by sys_init()
 */
void platform_detect()
{
    // things needed for isr_init()
    // for MultiCore, use acpi platform
    numcores=1;
}

/**
 * Detect devices on platform. Called by sys_init()
 */
void platform_enumerate()
{
// TODO: enumerate bus and load drivers
//    pci_init();
}

/**
 * map driver memory. Called by drv_init()
 */
virt_t platform_mapdrvmem()
{
    // x86 has a separate I/O address space with in/out instructions
    return 0;
}

/**
 * Power off the platform. Called by kprintf_poweroff()
 */
void platform_poweroff()
{
    // Bochs poweroff hack
    char *s = "Shutdown";
    while (*s) {
        asm volatile("movw $0x8900, %%dx; outb %0, %%dx" : : "a"(*s));
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
    asm volatile("movb $0xFE, %al; outb %al, $0x64");
}

/**
 * Hang computer.
 */
void platform_halt()
{
    asm volatile("1: cli; hlt; jmp 1b");
}

#if DEBUG
/**
 * initialize debug console
 */
void platform_dbginit()
{
    /* initialize uart as an alternative "keyboard" for debugger */
    asm volatile(
        "movw $0x3f9, %dx;"
        "xorb %al, %al;outb %al, %dx;"                   //IER int off
        "movb $0x80, %al;addb $2,%dl;outb %al, %dx;"     //LCR set divisor mode
        "movb $1, %al;subb $3,%dl;outb %al, %dx;"        //DLL divisor lo 115200
        "xorb %al, %al;incb %dl;outb %al, %dx;"          //DLH divisor hi
        "xorb %al, %al;incb %dl;outb %al, %dx;"          //FCR fifo off
        "movb $0x43, %al;incb %dl;outb %al, %dx;"        //LCR 8N1, break on
        "movb $0x8, %al;incb %dl;outb %al, %dx;"         //MCR Aux out 2
        "xorb %al, %al;subb $4,%dl;inb %dx, %al"         //clear receiver/transmitter
    );
    /* clear debug status */
    asm volatile("xorq %rax, %rax; movq %rax, %dr6");

}

/**
 * display a character on debug console
 */
void platform_dbgputc(uint8_t c)
{
    asm volatile(
        "xorl %%ebx, %%ebx; movb %0, %%bl;"
#ifndef NOBOCHSCONSOLE
        // bochs e9 port hack
        "movb %%bl, %%al;outb %%al, $0xe9;"
#endif
        // print character on serial
        "movl $10000,%%ecx;movw $0x3fd,%%dx;"
        // transmit buffer ready?
        "1:inb %%dx, %%al;"
        "cmpb $0xff,%%al;je 2f;"
        "dec %%ecx;jz 2f;"
        "andb $0x20,%%al;jz 1b;"
        // send out character
        "subb $5,%%dl;movb %%bl, %%al;outb %%al, %%dx;2:"
    ::"r"(c));
}
#endif
