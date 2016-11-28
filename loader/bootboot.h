/*
 * bootboot.h
 * 
 * Copyright 2016 BOOTBOOT bztsrc@github
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
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

#define BOOTPARAMS_MAGIC "BOOT"

#define FB_ARGB   0
#define FB_RGBA   1
#define FB_ABGR   2
#define FB_BGRA   3

#define INITRD_MAXSIZE 2 //Mb

typedef struct {
  uint8_t    magic[4];
  uint32_t   size;
  uint64_t   datetime;
  uint8_t   *acpi_ptr;
  uint8_t   *smbi_ptr;
  uint8_t   *efi_ptr;
  uint8_t   *mp_ptr;
  uint8_t   *initrd_ptr;
  uint64_t   initrd_size;
  uint8_t   *mmap_ptr;
  uint8_t   *fb_ptr;
  uint32_t   fb_size;
  uint32_t   fb_width;
  uint32_t   fb_height;
  uint32_t   fb_scanline;
  uint32_t   pagesize;
  uint8_t    protocol;
  uint8_t    loader_type;
  uint16_t   flags[2];
  int16_t    timezone;
  uint16_t   fb_type;
  uint16_t   mmap_num;
  uint8_t    mmap; /* MMapEnt[] */
} __attribute__((packed)) BOOTBOOT;

// mmap, type is stored in least significant byte of size
typedef struct {
  uint64_t   ptr;
  uint64_t   size;
} __attribute__((packed)) MMapEnt;

#define MMapEnt_Ptr(a)  (a->ptr)
#define MMapEnt_Size(a) (a->size & 0xFFFFFFFFFFFFFF00)
#define MMapEnt_Type(a) (a->size & 0xFF)
#define MMapEnt_IsFree(a) ((a->size&0xFF)==1||(a->size&0xFF)==3)

#define MMAP_FREE     1
#define MMAP_RESERVED 2
#define MMAP_ACPIFREE 3
#define MMAP_ACPINVS  4
#define MMAP_BAD      5
#define MMAP_MMIO     6

// minimum protocol level:
//  hardcoded kernel name, static kernel memory addresses
#define PROTOCOL_MINIMAL 0
// static protocol level:
//  kernel name parsed from environment, static kernel memory addresses
#define PROTOCOL_STATIC  1
// dynamic protocol level:
//  kernel name parsed from environment, kernel memory addresses parsed from ELF symbols
#define PROTOCOL_DYNAMIC 2

#define LOADER_BIOS 0
#define LOADER_UEFI 1

#ifndef _BOOTBOOT_LOADER
// import virtual addresses from linker
extern BOOTBOOT bootboot;
extern unsigned char environment[4096];
extern uint8_t fb;
#endif

#ifdef  __cplusplus
}
#endif

#endif
