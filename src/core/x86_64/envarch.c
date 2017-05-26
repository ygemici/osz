/*
 * core/x86_64/envarch.c
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
 * @brief Architecture specific environment parser
 */

#include <sys/sysinfo.h>

/*** for overriding default or autodetected values ***/
extern sysinfo_t sysinfostruc;
extern uint64_t ioapic_addr;

extern unsigned char *env_hex(unsigned char *s, uint64_t *v, uint64_t min, uint64_t max);
extern unsigned char *env_dec(unsigned char *s, uint64_t *v, uint64_t min, uint64_t max);

unsigned char *envarch_cs(unsigned char *s)
{
    uint64_t tmp;
    if(*s>='0' && *s<='9') {
        s = env_dec(s, &tmp, 0, 5);
        clocksource=(uint8_t)tmp;
        return s;
    }
    // skip separators
    while(*s==' '||*s=='\t')
        s++;
    if(s[0]=='h' && s[1]=='p')  clocksource=1;  // HPET
    if(s[0]=='p' && s[1]=='i')  clocksource=2;  // PIT
    if(s[0]=='r' && s[1]=='t')  clocksource=3;  // RTC
    if(s[0]=='l' && s[1]=='a')  clocksource=4;  // LAPIC
    if(s[0]=='x' && s[1]=='2')  clocksource=5;  // Lx2APIC
    while(*s!=0 && *s!='\n')
        s++;
    return s;
}

/*** initialize environment ***/
void envarch_init()
{
    // set up defaults
    sysinfostruc.systables[systable_hpet_idx] =
        sysinfostruc.systables[systable_apic_idx] =
            ioapic_addr = 0;
    sysinfostruc.systables[systable_dsdt_idx] = (uint64_t)fs_locate("etc/sys/dsdt");
    if(fs_size == 0)
        sysinfostruc.systables[systable_dsdt_idx] = 0;
}

// architecture specific parameters
unsigned char *envarch_parse(unsigned char *env)
{
	// manually override HPET address
	if(!kmemcmp(env, "hpet=", 5)) {
		env += 5;
		// we only accept hex value for this parameter
		env = env_hex(env, (uint64_t*)&sysinfostruc.systables[systable_hpet_idx], 1024*1024, 0);
	} else
	// manually override APIC address
	if(!kmemcmp(env, "apic=", 5)) {
		env += 5;
		// we only accept hex value for this parameter
		env = env_hex(env, (uint64_t*)&sysinfostruc.systables[systable_apic_idx], 1024*1024, 0);
	} else
	// manually override IOAPIC address
	if(!kmemcmp(env, "ioapic=", 7)) {
		env += 7;
		// we only accept hex value for this parameter
		env = env_hex(env, (uint64_t*)&ioapic_addr, 1024*1024, 0);
	} else
	// manually override clock source
	if(!kmemcmp(env, "clock=", 6)) {
		env += 6;
		env = envarch_cs(env);
	} else
		env++;
	return env;
}
