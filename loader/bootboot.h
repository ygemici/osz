/*
 * loader/bootboot.h
 *
 * Copyright 2017 Public Domain BOOTBOOT bztsrc@github
 *
 * This file is part of the BOOTBOOT Protocol package.
 * @brief The BOOTBOOT structure
 *
 */

#ifndef _BOOTBOOT_H_
#define _BOOTBOOT_H_

#ifdef  __cplusplus
extern "C" {
#endif

#define BOOTBOOT_MAGIC "BOOT"

// minimum protocol level:
//  hardcoded kernel name, static kernel memory addresses
#define PROTOCOL_MINIMAL 0
// static protocol level:
//  kernel name parsed from environment, static kernel memory addresses
#define PROTOCOL_STATIC  1
// dynamic protocol level:
//  kernel name parsed from environment, kernel memory addresses parsed from ELF symbols
#define PROTOCOL_DYNAMIC 2

// loader types, just informational
#define LOADER_BIOS 0
#define LOADER_UEFI 1
#define LOADER_RPI  2

// framebuffer pixel format, only 32 bits supported
#define FB_ARGB   0
#define FB_RGBA   1
#define FB_ABGR   2
#define FB_BGRA   3

// mmap entry, type is stored in least significant byte of size
typedef struct {
  uint64_t   ptr;
  uint64_t   size;
} __attribute__((packed)) MMapEnt;
#define MMapEnt_Ptr(a)  (a->ptr)
#define MMapEnt_Size(a) (a->size & 0xFFFFFFFFFFFFFFF0)
#define MMapEnt_Type(a) (a->size & 0xF)
#define MMapEnt_IsFree(a) ((a->size&0xF)==1||(a->size&0xF)==3)

#define MMAP_FREE     1
#define MMAP_RESERVED 2
#define MMAP_ACPIFREE 3
#define MMAP_ACPINVS  4
#define MMAP_USED     5
#define MMAP_MMIO     6

#define INITRD_MAXSIZE 16 //Mb

typedef struct {
  uint8_t    magic[4];    // BOOT, first 72 bytes architecture independent
  uint32_t   size;        // length of bootboot structure

  uint8_t    protocol;    // 1, static addresses, see PROTOCOL_* above
  uint8_t    loader_type; // see LOADER_* above
  uint16_t   flags;
  int16_t    timezone;    // in minutes -1440..1440
  uint16_t   fb_type;     // framebuffer type, see FB_* above

  uint32_t   pagesize;    // 4096
  uint32_t   bspid;       // Local APIC id of BSP

  uint8_t    datetime[8]; // in BCD yyyymmddhhiiss

  uint64_t   initrd_ptr;  // ramdisk image position and size
  uint64_t   initrd_size;

  uint8_t    *fb_ptr;     // framebuffer pointer and dimensions
  uint32_t   fb_size;
  uint32_t   fb_width;
  uint32_t   fb_height;
  uint32_t   fb_scanline;

  // architecture specific pointers, second 56 bytes arch dependent
  union {
    struct {
      uint64_t acpi_ptr;
      uint64_t smbi_ptr;
      uint64_t efi_ptr;
      uint64_t mp_ptr;
      uint64_t unused0;
      uint64_t unused1;
      uint64_t unused2;
    } x86_64;
    struct {
      uint64_t atags_ptr;
      uint64_t unused0;
      uint64_t unused1;
      uint64_t unused2;
      uint64_t unused3;
      uint64_t unused4;
      uint64_t unused5;
    } AArch64;
  };

  MMapEnt    mmap; /* MMapEnt[], more records may follow, all the rest of the page */
  /* use like this: MMapEnt *mmap_ent = &bootboot.mmap; mmap_ent++; */
} __attribute__((packed)) BOOTBOOT;


#ifdef  __cplusplus
}
#endif

#endif
