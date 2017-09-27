/*
 * core/x86_64/acpi/acpi.h
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
 * @brief ACPI structures
 */

typedef struct {
    uint8_t magic[8];
    uint8_t chksum;
    uint8_t oemid[6];
    uint8_t revision;
    uint32_t rsdt;
    uint32_t length;
    uint64_t xsdt;
    uint32_t echksum;
} __attribute__((packed)) ACPI_RSDPTR;

typedef struct {
    uint8_t magic[4];
    uint32_t length;
    uint8_t revision;
    uint8_t chksum;
    uint8_t oemid[6];
    uint8_t oemtableid[8];
    uint32_t oemrevision;
    uint32_t creatorid;
    uint32_t creatorrevision;
} __attribute__((packed)) ACPI_Header;

typedef struct {
    uint8_t magic[4];
    uint32_t length;
    uint8_t revision;
    uint8_t chksum;
    uint8_t oemid[6];
    uint8_t oemtableid[8];
    uint32_t oemrevision;
    uint32_t creatorid;
    uint32_t creatorrevision;
    uint32_t fw_ctrl;
    uint32_t dsdt;
    uint8_t reserved;
    uint8_t pref_pm_profile;
    uint16_t sci_int;
    uint32_t smi_cmd;
    uint8_t acpi_enable;
    uint8_t acpi_disable;
    uint8_t s4bios_req;
    uint8_t pstate_cnt;
    uint32_t PM1a_evt_blk;
    uint32_t PM1b_evt_blk;
    uint32_t PM1a_cnt_blk;
    uint32_t PM1b_cnt_blk;
    uint32_t PM2_cnt_blk;
    uint32_t PM_tmr_blk;
    uint32_t GPE0_blk;
    uint32_t GPE1_blk;
    uint8_t PM1_evt_len;
    uint8_t PM1_cnt_len;
    uint8_t PM2_cnt_len;
    uint8_t PM_tmr_len;
    uint8_t GPE0_blk_len;
    uint8_t GPE1_blk_len;
    uint8_t GPE1_base;
    uint8_t cst_cnt;
    uint16_t p_lvl2_lat;
    uint16_t p_lvl3_lat;
    uint16_t flush_size;
    uint16_t flush_stride;
    uint8_t duty_offset;
    uint8_t duty_width;
    uint8_t day_alrm;
    uint8_t mon_alrm;
    uint8_t century;
    uint16_t iapc_boot_arch;
    uint8_t reserved0;
    uint32_t flags;
    uint8_t reset_reg[12];
    uint8_t reset_value;
    uint8_t reserved1[3];
    uint64_t x_fw_ctrl;
    uint64_t x_dsdt;
} __attribute__((packed)) ACPI_FACP;

typedef struct {
    uint8_t magic[4];
    uint32_t length;
    uint8_t revision;
    uint8_t chksum;
    uint8_t oemid[6];
    uint8_t oemtableid[8];
    uint32_t oemrevision;
    uint32_t creatorid;
    uint32_t creatorrevision;
    uint32_t localApic;
    uint32_t flags;
} __attribute__((packed)) ACPI_APIC;

#define ACPI_APIC_LAPIC_MAGIC 0
typedef struct {
    uint8_t magic;
    uint8_t length;
    uint8_t procId;
    uint8_t apicId;
    uint32_t flags;
} __attribute__((packed)) ACPI_APIC_LAPIC;

#define ACPI_APIC_IOAPIC_MAGIC 1
typedef struct {
    uint8_t magic;
    uint8_t length;
    uint8_t ioApicId;
    uint8_t reserved;
    uint32_t address;
    uint32_t gsib;
} __attribute__((packed)) ACPI_APIC_IOAPIC;

#define ACPI_APIC_INTSRCOV_MAGIC 2
typedef struct {
    uint8_t magic;
    uint8_t length;
    uint8_t bus;
    uint8_t source;
    uint32_t gsi;
    uint16_t flags;
} __attribute__((packed)) ACPI_APIC_INTSRCOV;

#define ACPI_APIC_NMI_MAGIC 3
typedef struct {
    uint8_t magic;
    uint8_t length;
    uint16_t flags;
    uint32_t gsi;
} __attribute__((packed)) ACPI_APIC_NMI;

#define ACPI_APIC_LNMI_MAGIC 4
typedef struct {
    uint8_t magic;
    uint8_t length;
} __attribute__((packed)) ACPI_APIC_LNMI;

#define ACPI_APIC_LADDROV_MAGIC 5
typedef struct {
    uint8_t magic;
    uint8_t length;
} __attribute__((packed)) ACPI_APIC_LADDROV;

#define ACPI_APIC_IOSAPIC_MAGIC 6
typedef struct {
    uint8_t magic;
    uint8_t length;
} __attribute__((packed)) ACPI_APIC_IOSAPIC;

#define ACPI_APIC_LSAPIC_MAGIC 7
typedef struct {
    uint8_t magic;
    uint8_t length;
} __attribute__((packed)) ACPI_APIC_LSAPIC;

#define ACPI_APIC_PIS_MAGIC 8
typedef struct {
    uint8_t magic;
    uint8_t length;
} __attribute__((packed)) ACPI_APIC_PIS;

#define ACPI_APIC_X2APIC_MAGIC 9
typedef struct {
    uint8_t magic;
    uint8_t length;
} __attribute__((packed)) ACPI_APIC_X2APIC;

#define ACPI_APIC_X2NMI_MAGIC 10
typedef struct {
    uint8_t magic;
    uint8_t length;
} __attribute__((packed)) ACPI_APIC_X2NMI;

typedef struct {
    uint8_t magic[4];
    uint32_t length;
    uint8_t revision;
    uint8_t chksum;
    uint8_t oemid[6];
    uint8_t oemtableid[8];
    uint32_t oemrevision;
    uint32_t creatorid;
    uint32_t creatorrevision;
    uint32_t hwId;
    uint8_t base[12];
    uint8_t hpet_no;
    uint16_t minClkTickPeriodic;
    uint8_t pgProt;
} __attribute__((packed)) ACPI_HPET;
