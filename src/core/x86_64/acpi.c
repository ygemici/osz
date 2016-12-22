/*
 * core/x86_64/acpi.c
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
 * @brief ACPI table parser
 */

#include "../core.h"
#include "../env.h"
#include "acpi.h"

uint64_t __attribute__ ((section (".data"))) lapic_addr;
uint64_t __attribute__ ((section (".data"))) ioapic_addr;
uint64_t __attribute__ ((section (".data"))) hpet_addr;

char *acpi_getdriver(char *device, char *drvs, char *drvs_end)
{
    return NULL;
}

/* reentrant acpi table parser */
void acpi_parse(ACPI_Header *hdr, uint64_t level)
{
    char *ptr = (char*)hdr;
    uint32_t i,len = hdr->length - sizeof(ACPI_Header);
    uint64_t data = (uint64_t)((char*)hdr + sizeof(ACPI_Header));

    /* maximum tree depth */
    if(level>8)
        return;
#if DEBUG
    if(debug==DBG_SYSTABLES) {
        for(i=0;i<level;i++) kprintf("  ");
        kprintf("%c%c%c%c ", hdr->magic[0], hdr->magic[1], hdr->magic[2], hdr->magic[3]);
        kprintf("%x %d\n", hdr, hdr->length);
    }
#endif
    /* System tables pointer, should never get it, but just in case */
    if(!kmemcmp("RSD PTR ", hdr->magic, 8)) {
        ACPI_RSDPTR *rsdptr = (ACPI_RSDPTR *)hdr;
        ptr = (char*)rsdptr->xsdt;
        if(ptr==0)
            ptr = (char*)((uint64_t)(*((uint32_t*)&rsdptr->rsdt)));
        acpi_parse((ACPI_Header*)ptr, 0);
    } else
    /* System tables */
    if(!kmemcmp("RSDT", hdr->magic, 4) || !kmemcmp("XSDT", hdr->magic, 4)) {
        ptr = 0;
        while(len>0) {
            if(hdr->magic[0]=='X') {
                ptr = (char*)(*((uint64_t*)data));
                data += 8;
                len -= 8;
            } else {
                ptr = (char*)((uint64_t)(*((uint32_t*)data)));
                data += 4;
                len -= 4;
            }
            acpi_parse((ACPI_Header*)ptr, level+1);
        }
    } else
    /* Fixed ACPI Description Table */
    if(!kmemcmp("FACP", hdr->magic, 4)) {
//        ACPI_FACP *fadt = (ACPI_FACP *)hdr;
        // TODO: get values for poweroff
    } else
    /* Multiple APIC Description Table */
    if(!kmemcmp("APIC", hdr->magic, 4)) {
        ACPI_APIC *madt = (ACPI_APIC *)hdr;
        lapic_addr = madt->localApic;
        // This header is 8 bytes longer than normal header
        len -= 8; ptr = (char*)(data+8);
        while(len>0) {
            if(ptr[0]==ACPI_APIC_IOAPIC_MAGIC) {
                ACPI_APIC_IOAPIC *rec = (ACPI_APIC_IOAPIC*)ptr;
#if DEBUG
                if(debug==DBG_SYSTABLES) {
                    for(i=0;i<=level;i++) kprintf("  ");
                    kprintf("IOAPIC  %x %d %x\n", ptr, ptr[1], rec->address);
                }
#endif
                ioapic_addr = rec->address;
#if DEBUG
            } else
            if(ptr[0]==ACPI_APIC_LAPIC_MAGIC) {
                ACPI_APIC_LAPIC *rec = (ACPI_APIC_LAPIC*)ptr;
                if(debug==DBG_SYSTABLES) {
                    for(i=0;i<=level;i++) kprintf("  ");
                    kprintf("LAPIC   %x %d %d\n", ptr, ptr[1], rec->procId);
                }
            } else
            if(ptr[0]==ACPI_APIC_INTSRCOV_MAGIC) {
                if(debug==DBG_SYSTABLES) {
                    for(i=0;i<=level;i++) kprintf("  ");
                    kprintf("INTSRC  %x %d\n", ptr, ptr[1]);
                }
            } else
            if(ptr[0]==ACPI_APIC_NMI_MAGIC) {
                if(debug==DBG_SYSTABLES) {
                    for(i=0;i<=level;i++) kprintf("  ");
                    kprintf("NMI     %x %d\n", ptr, ptr[1]);
                }
            } else
            if(ptr[0]==ACPI_APIC_LNMI_MAGIC) {
                if(debug==DBG_SYSTABLES) {
                    for(i=0;i<=level;i++) kprintf("  ");
                    kprintf("LNMI    %x %d\n", ptr, ptr[1]);
                }
            } else
            if(ptr[0]==ACPI_APIC_LADDROV_MAGIC) {
                if(debug==DBG_SYSTABLES) {
                    for(i=0;i<=level;i++) kprintf("  ");
                    kprintf("LADDR   %x %d\n", ptr, ptr[1]);
                }
            } else
            if(ptr[0]==ACPI_APIC_IOSAPIC_MAGIC) {
                if(debug==DBG_SYSTABLES) {
                    for(i=0;i<=level;i++) kprintf("  ");
                    kprintf("IOSAPIC %x %d\n", ptr, ptr[1]);
                }
            } else
            if(ptr[0]==ACPI_APIC_LSAPIC_MAGIC) {
                if(debug==DBG_SYSTABLES) {
                    for(i=0;i<=level;i++) kprintf("  ");
                    kprintf("LSAPIC  %x %d\n", ptr, ptr[1]);
                }
            } else
            if(ptr[0]==ACPI_APIC_PIS_MAGIC) {
                if(debug==DBG_SYSTABLES) {
                    for(i=0;i<=level;i++) kprintf("  ");
                    kprintf("PIS     %x %d\n", ptr, ptr[1]);
                }
            } else
            if(ptr[0]==ACPI_APIC_X2APIC_MAGIC) {
                if(debug==DBG_SYSTABLES) {
                    for(i=0;i<=level;i++) kprintf("  ");
                    kprintf("X2APIC  %x %d\n", ptr, ptr[1]);
                }
            } else
            if(ptr[0]==ACPI_APIC_X2NMI_MAGIC) {
                if(debug==DBG_SYSTABLES) {
                    for(i=0;i<=level;i++) kprintf("  ");
                    kprintf("X2NMI   %x %d\n", ptr, ptr[1]);
                }
#endif
            }
            len-=ptr[1];
            ptr+=ptr[1];
        }
    } else
    /* High Precision Event Timer */
    if(!kmemcmp("HPET", hdr->magic, 4)) {
        hpet_addr = (uint64_t)hdr;
    }
}

void dev_init()
{
    // this is so early, we don't have initrd in fs process' bss yet.
    // so we have to rely on identity mapping to locate the files
    char *s, *f, *drvs = (char *)fs_locate("etc/sys/drivers");
    char *drvs_end = drvs + fs_size;
    char fn[256];
    hpet_addr=0;

    if(drvs==NULL ||
        bootboot.acpi_ptr==0 ||
        ((char)(*((uint64_t*)bootboot.acpi_ptr))!='R' &&
        (char)(*((uint64_t*)bootboot.acpi_ptr))!='X')
    ) {
        // should never happen!
        kprintf("core - W - missing %s\n", drvs==NULL?"/etc/sys/drivers":"ACPI tables");
        // hardcoded legacy devices if driver list not found
        service_loadso("lib/sys/input/ps2.so");
        service_loadso("lib/sys/display/fb.so");
        service_loadso("lib/sys/proc/pitrtc.so");
    } else {
        // load devices which don't have entry in any ACPI tables
        for(s=drvs;s<drvs_end;) {
            f = s; while(s<drvs_end && *s!=0 && *s!='\n') s++;
            // skip filesystem drivers
            if(f[0]=='*' && f[1]==9 && (f[2]!='f' || f[3]!='s')) {
                f+=2;
                if(s-f<255-8) {
                    kmemcpy(&fn[0], "lib/sys/", 8);
                    kmemcpy(&fn[8], f, s-f);
                    fn[s-f+8]=0;
                    service_loadso(fn);
                }
                continue;
            }
            // failsafe
            if(s>=drvs_end || *s==0) break;
            if(*s=='\n') s++;
        }
        // recursively parse ACPI tables to detect devices
#if DEBUG
        if(debug==DBG_SYSTABLES)
            kprintf("ACPI system table %x\n", bootboot.acpi_ptr);
#endif
        acpi_parse((ACPI_Header*)bootboot.acpi_ptr, 0);
        // fallback to default if not found and not given either
        if(ioapic_addr==0)
            ioapic_addr=0xFEC00000;
        // load timer driver
        service_loadso(hpet_addr==0?
            "lib/sys/proc/pitrtc.so":
            "lib/sys/proc/hpet.so");
    }
}

void dev_poweroff()
{
    // disable all interrupts and switch to identity mapping
    // so that ACPI tables will be at expected position
    isr_disable();
    // TODO: APCI poweroff
    __asm__ __volatile__ ( "xchgw %%bx,%%bx;cli;hlt" : : : );
}
