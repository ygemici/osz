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

#include "acpi.h"
#include "isr.h"

/* main properties, configurable */
uint64_t __attribute__ ((section (".data"))) hpet_addr;
uint64_t __attribute__ ((section (".data"))) dsdt_addr;
uint64_t __attribute__ ((section (".data"))) lapic_addr;
uint64_t __attribute__ ((section (".data"))) ioapic_addr;

/* poweroff stuff, autodetected */
uint32_t __attribute__ ((section (".data"))) SLP_EN;
uint32_t __attribute__ ((section (".data"))) SLP_TYPa;
uint32_t __attribute__ ((section (".data"))) PM1a_CNT;
uint32_t __attribute__ ((section (".data"))) SLP_TYPb;
uint32_t __attribute__ ((section (".data"))) PM1b_CNT;

extern uint64_t isr_entropy[4];
extern uint32_t fg;
extern char poweroffprefix[];
extern char poweroffsuffix[];
extern void kprintf_center(int w, int h);
extern void pci_init();

/* pre-parse ACPI tables. Detect IOAPIC and HPET address */
void acpi_early(ACPI_Header *hdr)
{
    /* get root pointer */
    if(hdr==NULL)
        hdr = (ACPI_Header*)bootboot.acpi_ptr;
    char *ptr = (char*)hdr;
    uint32_t len = hdr->length - sizeof(ACPI_Header);
    uint64_t data = (uint64_t)((char*)hdr + sizeof(ACPI_Header));
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
            acpi_early((ACPI_Header*)ptr);
        }
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
                ioapic_addr = rec->address;
                break;
            }
            len-=ptr[1];
            ptr+=ptr[1];
        }
    } else
    /* High Precision Event Timer */
    if(!hpet_addr && !kmemcmp("HPET", hdr->magic, 4)) {
        hpet_addr = (uint64_t)hdr;
    }
}

/* DSDT and SSDT parsing */
void acpi_parsesdt(ACPI_Header *hdr)
{
}

/* reentrant acpi table parser */
void acpi_parse(ACPI_Header *hdr, uint64_t level)
{
    char *ptr = (char*)hdr;
    uint32_t len = hdr->length - sizeof(ACPI_Header);
    uint64_t data = (uint64_t)((char*)hdr + sizeof(ACPI_Header));
    /* add entropy */
    isr_entropy[(len+0)%4] += (uint64_t)hdr;
    isr_entropy[(len+1)%4] -= (uint64_t)hdr;
    isr_entropy[(len+2)%4] += ((uint64_t)hdr<<1);
    isr_entropy[(len+4)%4] -= (uint64_t)((uint64_t)hdr>>1);
    isr_gainentropy();

    /* maximum tree depth */
    if(level>8)
        return;
#if DEBUG
    uint64_t i;
    if(debug&DBG_DEVICES) {
        for(i=0;i<level;i++) kprintf("  ");
        kprintf("%c%c%c%c ", hdr->magic[0], hdr->magic[1], hdr->magic[2], hdr->magic[3]);
        kprintf("%8x %d ", hdr, hdr->length);
        kprintf("%c%c%c%c\n", hdr->oemid[0], hdr->oemid[1], hdr->oemid[2], hdr->oemid[3]);
    }
#endif
    /* System tables pointer, should never get it, but just in case failsafe */
    if(!kmemcmp("RSD PTR ", hdr->magic, 8)) {
        ACPI_RSDPTR *rsdptr = (ACPI_RSDPTR *)hdr;
        ptr = (char*)rsdptr->xsdt;
        if(ptr==0)
            ptr = (char*)((uint64_t)(*((uint32_t*)&rsdptr->rsdt)));
        acpi_parse((ACPI_Header*)ptr, 1);
    } else
    /* System tables */
    if(!kmemcmp("RSDT", hdr->magic, 4) || !kmemcmp("XSDT", hdr->magic, 4)) {
        char tmp[9];
        kmemcpy((char*)&tmp[0],(char*)&hdr->oemid[0],8); tmp[9]=0;
        syslog_early("%cSDT %x %s",(uint8_t)hdr->magic[0],hdr,&tmp[0]);
        /* defaults for qemu and bochs */
        if(!kmemcmp(hdr->oemid,"BOCHS",5)) {
            PM1a_CNT = 0xB004;
            SLP_EN = 0x2000;
        }
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
        ACPI_FACP *fadt = (ACPI_FACP *)hdr;
        // TODO: get values for poweroff
        if(PM1a_CNT==0) {
            PM1a_CNT = fadt->PM1a_cnt_blk;
            PM1b_CNT = fadt->PM1b_cnt_blk;
        }
    } else
    /* Specific Description Tables */
    if(!kmemcmp("DSDT", hdr->magic, 4) || !kmemcmp("SSDT", hdr->magic, 4)) {
        if(dsdt_addr == 0)
            dsdt_addr = (uint64_t)hdr;
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
                if(debug&DBG_DEVICES) {
                    for(i=0;i<=level;i++) kprintf("  ");
                    kprintf("IOAPIC  %x %d %x\n", ptr, ptr[1], rec->address);
                }
#endif
                if(!ioapic_addr)
                    ioapic_addr = rec->address;
#if DEBUG
            } else
            if(ptr[0]==ACPI_APIC_LAPIC_MAGIC) {
                ACPI_APIC_LAPIC *rec = (ACPI_APIC_LAPIC*)ptr;
                if(debug&DBG_DEVICES) {
                    for(i=0;i<=level;i++) kprintf("  ");
                    kprintf("LAPIC   %x %d %d\n", ptr, ptr[1], rec->procId);
                }
            } else
            if(ptr[0]==ACPI_APIC_INTSRCOV_MAGIC) {
                if(debug&DBG_DEVICES) {
                    for(i=0;i<=level;i++) kprintf("  ");
                    kprintf("INTSRC  %x %d\n", ptr, ptr[1]);
                }
            } else
            if(ptr[0]==ACPI_APIC_NMI_MAGIC) {
                if(debug&DBG_DEVICES) {
                    for(i=0;i<=level;i++) kprintf("  ");
                    kprintf("NMI     %x %d\n", ptr, ptr[1]);
                }
            } else
            if(ptr[0]==ACPI_APIC_LNMI_MAGIC) {
                if(debug&DBG_DEVICES) {
                    for(i=0;i<=level;i++) kprintf("  ");
                    kprintf("LNMI    %x %d\n", ptr, ptr[1]);
                }
            } else
            if(ptr[0]==ACPI_APIC_LADDROV_MAGIC) {
                if(debug&DBG_DEVICES) {
                    for(i=0;i<=level;i++) kprintf("  ");
                    kprintf("LADDR   %x %d\n", ptr, ptr[1]);
                }
            } else
            if(ptr[0]==ACPI_APIC_IOSAPIC_MAGIC) {
                if(debug&DBG_DEVICES) {
                    for(i=0;i<=level;i++) kprintf("  ");
                    kprintf("IOSAPIC %x %d\n", ptr, ptr[1]);
                }
            } else
            if(ptr[0]==ACPI_APIC_LSAPIC_MAGIC) {
                if(debug&DBG_DEVICES) {
                    for(i=0;i<=level;i++) kprintf("  ");
                    kprintf("LSAPIC  %x %d\n", ptr, ptr[1]);
                }
            } else
            if(ptr[0]==ACPI_APIC_PIS_MAGIC) {
                if(debug&DBG_DEVICES) {
                    for(i=0;i<=level;i++) kprintf("  ");
                    kprintf("PIS     %x %d\n", ptr, ptr[1]);
                }
            } else
            if(ptr[0]==ACPI_APIC_X2APIC_MAGIC) {
                if(debug&DBG_DEVICES) {
                    for(i=0;i<=level;i++) kprintf("  ");
                    kprintf("X2APIC  %x %d\n", ptr, ptr[1]);
                }
            } else
            if(ptr[0]==ACPI_APIC_X2NMI_MAGIC) {
                if(debug&DBG_DEVICES) {
                    for(i=0;i<=level;i++) kprintf("  ");
                    kprintf("X2NMI   %x %d\n", ptr, ptr[1]);
                }
#endif
            }
            len-=ptr[1];
            ptr+=ptr[1];
        }
    }
}

/* Load device drivers into "system" task's address space */
void acpi_init()
{
    SLP_EN = PM1a_CNT = 0;

    if(bootboot.acpi_ptr==0 ||
        ((char)(*((uint64_t*)bootboot.acpi_ptr))!='R' &&
        (char)(*((uint64_t*)bootboot.acpi_ptr))!='X')
    ) {
        // should never happen!
#if DEBUG
        kprintf("WARNING no ACPI tables found\n");
#endif
    } else {
        // recursively parse ACPI tables to detect devices
#if DEBUG
        if(debug&DBG_DEVICES)
            kprintf("\nACPI System Tables\n");
#endif
        acpi_parse((ACPI_Header*)bootboot.acpi_ptr, 1);
    }
    if(dsdt_addr != 0)
        acpi_parsesdt((ACPI_Header*)dsdt_addr);
    // fallback to default if not found and not given either
    if(ioapic_addr==0)
        ioapic_addr=0xFEC00000;
    /* load timer driver */
    //   with PIC:  PIT and RTC
    //   with APIC: HPET, if detected, APIC timer otherwise
    //   with x2APIC: HPET, if detected, x2APIC timer otherwise
    service_loadso(hpet_addr==0 || (hpet_addr & (__PAGESIZE-1))!=0?
        (ISR_CTRL==CTRL_PIC?"lib/sys/proc/pitrtc.so":
        (ISR_CTRL==CTRL_APIC?"lib/sys/proc/apic.so":
            "lib/sys/proc/x2apic.so")):
        "lib/sys/proc/hpet.so");
}

void acpi_poweroff()
{
    // APCI poweroff
/*
    if(PM1a_CNT!=0) {
        int en = SLP_TYPa | SLP_EN;
        //acpi_enable();
        breakpoint;
        __asm__ __volatile__ (
            "movq %0, %%rax; movq %1, %%rdx; outw %%ax, %%dx" : :
            "m"(en), "m"(PM1a_CNT) );
        if ( PM1b_CNT != 0 ) {
            en = SLP_TYPb | SLP_EN;
            __asm__ __volatile__ (
                "movq %0, %%rax; movq %1, %%rdx; outw %%ax, %%dx" : :
                "m"(en), "m"(PM1b_CNT) );
        }
    }
*/
}
