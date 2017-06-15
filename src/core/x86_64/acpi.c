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

#include <elf.h>
#include "acpi.h"
#include "isr.h"

/* poweroff stuff, autodetected */
uint32_t __attribute__ ((section (".data"))) SLP_EN;
uint32_t __attribute__ ((section (".data"))) SLP_TYPa;
uint32_t __attribute__ ((section (".data"))) PM1a_CNT;
uint32_t __attribute__ ((section (".data"))) SLP_TYPb;
uint32_t __attribute__ ((section (".data"))) PM1b_CNT;
uint32_t __attribute__ ((section (".data"))) PM1_CNT_LEN;
uint32_t __attribute__ ((section (".data"))) ACPI_ENABLE;
uint32_t __attribute__ ((section (".data"))) SCI_EN;
uint32_t __attribute__ ((section (".data"))) SCI_INT;
uint32_t __attribute__ ((section (".data"))) SMI_CMD;

extern sysinfo_t sysinfostruc;
extern uint64_t ioapic_addr;
extern uint64_t hpet_addr;

/**
 * pre-parse ACPI tables. Detect IOAPIC and HPET address, called by isr_init()
 */
void acpi_early(ACPI_Header *hdr)
{
    /* get root pointer */
    if(hdr==NULL)
        hdr = (ACPI_Header*)bootboot.x86_64.acpi_ptr;
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
        sysinfostruc.systables[systable_apic_idx] = madt->localApic;
        // This header is 8 bytes longer than normal header
        len -= 8; ptr = (char*)(data+8);
        while(len>0) {
            if(ptr[0]==ACPI_APIC_IOAPIC_MAGIC) {
                ACPI_APIC_IOAPIC *rec = (ACPI_APIC_IOAPIC*)ptr;
                ioapic_addr = rec->address;
#if DEBUG
                if(debug&DBG_DEVICES)
                    kprintf("IOAPIC  %x\n", ioapic_addr);
#endif
                break;
            }
            len-=ptr[1];
            ptr+=ptr[1];
        }
    } else
    /* High Precision Event Timer */
    if(!hpet_addr && !kmemcmp("HPET", hdr->magic, 4)) {
        hpet_addr = (uint64_t)hdr;
#if DEBUG
        if(debug&DBG_DEVICES)
            kprintf("HPET  %x\n", hpet_addr);
#endif
    }
}

/**
 * DSDT and SSDT parsing
 */
void acpi_parseaml(ACPI_Header *hdr)
{
#if DEBUG
        if(debug&DBG_DEVICES) {
            kprintf("AML %c%c%c%c  %x\n",
                hdr->magic[0], hdr->magic[1], hdr->magic[2], hdr->magic[3],
                hdr);
        }
#endif
    // search the \_S5 package in the DSDT
    char *c = (char *)hdr + sizeof(ACPI_Header);
    int len = (uint32_t)hdr->length;

    // locate _S5_
    for (;(c[0]!=0x8 || c[1]!='_' || c[2]!='S' || c[3]!='5' || c[4]!='_') && len > 0; len--)
      c++;
    c += 5;

    // check if _S5_ was found
    if ( len>0 && *c == 0x12 )
    {
        c++;
        if (*c == 0x06 || *c == 0x0A) {
            c++;   // skip byteprefix
            SLP_TYPa = *(c)<<10;
            c++;
        }
        if (*c == 0x0A) {
            c++;   // skip byteprefix
            SLP_TYPb = *(c)<<10;
            c++;
        }
    }
}

/**
 * reentrant acpi table parser
 */
void acpi_parse(ACPI_Header *hdr, uint64_t level)
{
    char *ptr = (char*)hdr;
    uint32_t len = hdr->length - sizeof(ACPI_Header);
    uint64_t data = (uint64_t)((char*)hdr + sizeof(ACPI_Header));
    /* add entropy */
    sysinfostruc.srand[(len+0)%4] += (uint64_t)hdr;
    sysinfostruc.srand[(len+1)%4] -= (uint64_t)hdr;
    sysinfostruc.srand[(len+2)%4] += ((uint64_t)hdr<<1);
    sysinfostruc.srand[(len+4)%4] -= (uint64_t)((uint64_t)hdr>>1);
    kentropy();

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
    /* System tables pointer, should never get it, but just in case, failsafe */
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
        // get values for poweroff
        if(PM1a_CNT==0) {
            PM1a_CNT = fadt->PM1a_cnt_blk;
            PM1b_CNT = fadt->PM1b_cnt_blk;
            PM1_CNT_LEN = fadt->PM1_cnt_len;
            SMI_CMD = fadt->smi_cmd;
            ACPI_ENABLE = fadt->acpi_enable;
            SLP_EN = 1<<13;
            SCI_INT = fadt->sci_int;
            SCI_EN = 1;
        }
        hdr = (ACPI_Header*)(fadt->x_dsdt && (fadt->x_dsdt>>48)==0 ? fadt->x_dsdt : fadt->dsdt);
        if(!kmemcmp("SDT", hdr->magic+1, 3) && sysinfostruc.systables[systable_dsdt_idx] == 0)
            sysinfostruc.systables[systable_dsdt_idx] = (uint64_t)hdr;
    } else
    /* Specific Description Tables */
    if(!kmemcmp("DSDT", hdr->magic, 4) || !kmemcmp("SSDT", hdr->magic, 4)) {
        if(sysinfostruc.systables[systable_dsdt_idx] == 0)
            sysinfostruc.systables[systable_dsdt_idx] = (uint64_t)hdr;
    } else
    /* Multiple APIC Description Table */
    if(!kmemcmp("APIC", hdr->magic, 4)) {
        // This header is 8 bytes longer than normal header
        len -= 8; ptr = (char*)(data+8);
        while(len>0) {
#if DEBUG
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

/**
 * Load device drivers according to System Tables. Called by sys_init()
 */
bool_t acpi_init()
{
    uint64_t i = 10000;
    uint16_t tmp;
    SLP_EN = PM1a_CNT = 0;

    if(bootboot.x86_64.acpi_ptr==0 ||
        ((char)(*((uint64_t*)bootboot.x86_64.acpi_ptr))!='R' &&
        (char)(*((uint64_t*)bootboot.x86_64.acpi_ptr))!='X')
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
        acpi_parse((ACPI_Header*)bootboot.x86_64.acpi_ptr, 1);
    }
    if(sysinfostruc.systables[systable_dsdt_idx] != 0)
        acpi_parseaml((ACPI_Header *)sysinfostruc.systables[systable_dsdt_idx]);
    // fallback to default if not found and not given either
    if(ioapic_addr==0)
        ioapic_addr=0xFEC00000;

    if(PM1a_CNT==0)
        return false;
    if(SCI_INT!=0) {
//        Elf64_Ehdr *ehdr = (Elf64_Ehdr *)drv_init("lib/sys/proc/sci.so");
//        ehdr->e_machine = SCI_INT;
    }
#if DEBUG
    if(debug&DBG_DEVICES)
        kprintf("ACPI PM1a_CNT %x SMI_CMD %x ACPI_ENABLE %x SLP_TYPa %x SLP_EN %x\n", 
            PM1a_CNT, SMI_CMD, ACPI_ENABLE, SLP_TYPa,SLP_EN);
#endif
    // check if acpi is enabled
    __asm__ __volatile__ (
        "xor %%rax, %%rax;movl %1, %%edx; inw %%dx, %%ax" :
        "=r"(tmp) :
        "r"(PM1a_CNT) );
    if ( (tmp &SCI_EN) == 0 ) {
        // enable
        if (SMI_CMD != 0 && ACPI_ENABLE != 0) {
            __asm__ __volatile__ (
                "movl %0, %%eax; movl %1, %%edx; outw %%ax, %%dx" : :
                "r"(ACPI_ENABLE), "r"(SMI_CMD) );
            do {
                __asm__ __volatile__ (
                    "xor %%rax, %%rax;movl %1, %%edx; inw %%dx, %%ax" :
                    "=r"(tmp) :
                    "r"(PM1a_CNT) );
            } while(i-- > 0 && (tmp &SCI_EN)==0);
            if (PM1b_CNT != 0)
                do {
                    __asm__ __volatile__ (
                        "xor %%rax, %%rax;movl %1, %%edx; inw %%dx, %%ax" :
                        "=r"(tmp) :
                        "r"(PM1b_CNT) );
                } while(i-- > 0 && (tmp &SCI_EN)==0);
#if DEBUG
        if(debug&DBG_DEVICES) {
            kprintf("ACPI enable %d\n",i>0);
        }
#endif
        return i>0;
      } else
        return false;
    } else {
#if DEBUG
        if(debug&DBG_DEVICES) {
            kprintf("ACPI already enabled\n");
        }
#endif
    }
    return true;
}

/**
 * Power off the computer. Called by sys_disable()
 */
void acpi_poweroff()
{
    uint64_t en;
    if(PM1a_CNT!=0) {
        en = SLP_TYPa | SLP_EN;
        __asm__ __volatile__ (
            "movq %0, %%rax; movl %1, %%edx; outw %%ax, %%dx" : :
            "r"(en), "r"(PM1a_CNT) );
        if ( PM1b_CNT != 0 ) {
            en = SLP_TYPb | SLP_EN;
            __asm__ __volatile__ (
                "movq %0, %%rax; movl %1, %%edx; outw %%ax, %%dx" : :
                "r"(en), "r"(PM1b_CNT) );
        }
    }
}
