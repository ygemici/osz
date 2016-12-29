;*
;* loader/mb-x86_64/bootboot.inc
;*
;* Copyright 2016 Public Domain BOOTBOOT bztsrc@github
;*
;* This file is part of the BOOTBOOT Protocol package.
;* @brief The BOOTBOOT structure
;*
; ------ !!! WARNING: MUST MATCH ../bootboot.h !!! ------

bootboot = 8000h



; this define is in the 32th line of bootboot.h
bootboot_MAGIC     equ 'BOOT'

FB_ARGB       equ 0
FB_RGBA       equ 1
FB_ABGR       equ 2
FB_BGRA       equ 3

INITRD_MAXSIZE     equ 2 ; Mb

virtual at bootboot
  bootboot.magic:       dd	0
  bootboot.size:        dd	0
  bootboot.datetime:    dq	0
  bootboot.acpi_ptr:    dq	0
  bootboot.smbi_ptr:    dq	0
  bootboot.efi_ptr:     dq	0
  bootboot.mp_ptr:      dq	0
  bootboot.initrd_ptr:  dq	0
  bootboot.initrd_size: dq	0
  bootboot.mmap_ptr:    dq	0
  bootboot.unused:      dd	0,0,0
  bootboot.bspid:       dd	0
  bootboot.fb_ptr:      dq	0
  bootboot.fb_size:     dd	0
  bootboot.fb_width:    dd	0
  bootboot.fb_height:   dd	0
  bootboot.fb_scanline: dd	0
  bootboot.pagesize:    dd	0
  bootboot.protocol_ver:db  1
  bootboot.loader_type: db  0
  bootboot.flags:       dw	0,0,0
  bootboot.timezone:    dw	0
  bootboot.fb_type:     dw	0
  bootboot.mmap:
end virtual

;  mmap entry, type is stored in least significant byte of size
virtual at 0
    mmap_ent.ptr:         dq	0
    mmap_ent.size:        dq	0
end virtual
; realsize = size & 0xFFFFFFFFFF00
; type = size & 0xFF
















