;*
;* fs.inc
;*
;* Copyright 2016 BOOTBOOT bztsrc@github
;*
;* This program is free software; you can redistribute it and/or modify
;* it under the terms of the GNU General Public License as published by
;* the Free Software Foundation; either version 2 of the License, or
;* (at your option) any later version.
;* 
;* This program is distributed in the hope that it will be useful,
;* but WITHOUT ANY WARRANTY; without even the implied warranty of
;* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;* GNU General Public License for more details.
;*
;* You should have received a copy of the GNU General Public License
;* along with this program; if not, write to the Free Software
;* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
;* MA 02110-1301, USA.
;*
;* This file is part of the BOOTBOOT Protocol package.
;* @brief Filesystem drivers for initial ramdisk.
;*

;*********************************************************************
;*                       File System Drivers                         *
;*********************************************************************

; unlike the EFI loader here we have to parse the partition to locate
; our data in it. So there's an extra method to every filesystem driver.
; methods:
;  *_1stpart_initrd: get initrd (not in EFI)
;  *_initrd: get kernel (common with EFI)

			USE32
fsdrivers:	dw			gzip_1stpart_initrd, gzip_initrd
			dw			fsz_1stpart_initrd, fsz_initrd
			dw			cpio_1stpart_initrd, cpio_initrd
			dw			tar_1stpart_initrd, tar_initrd
			;dw			fat12_1stpart_initrd, fat12_initrd
			;dw			fat16_1stpart_initrd, fat16_initrd
			dw			0, 0, 0

; ----------- FS/Z ----------
; Find the initrd on EFI System Partition
; IN:   esi: 1st partition pointer, ecx: 1st partition end
; OUT:  On Success
;         esi: pointer to initrd, ecx: size in bytes
;       On Error
;         esi: error message, ecx: 0
fsz_1stpart_initrd:
			mov			ebx, ecx
			xor			ecx, ecx
			mov			eax, 'FS/Z'
@@:			add			esi, 512
			cmp			dword [esi+512], eax	; FSZ_SuperBlock.magic
			jne			.not
			cmp			dword [esi+1016], eax	; FSZ_SuperBlock.magic2
			je			@f
.not:		cmp			esi, ebx
			jb			@b
			xor			ecx, ecx
			ret
			; get total number of sectors
@@:			mov			ecx, dword [esi+520]
			; sec*4096
			shl			ecx, 12
			ret

; Find the ELF kernel on initrd
; IN:   esi: initrd pointer, ecx: initrd end, edi: kernel filename
; OUT:  On Success
;         esi: pointer to an ELF64 executable, ecx: size in bytes
;       On Error
;         esi: error message, ecx: 0
fsz_initrd:
			mov			ebx, ecx
			xor			ecx, ecx
			; FS/Z superblock
			; get root dir inode
			mov			eax, dword [esi+552]	; FSZ_SuperBlock.rootdirfid
			shl			eax, 12
			add			esi, eax
			cmp			dword [esi], 'FSIN'
			je			@f
.noroot:	mov			esi, noroot
.err:		xor			ecx, ecx
			ret
.nolib:		mov			esi, nolib
			jmp			.err
.nocore:	mov			esi, nocore
			jmp			.err
@@:			; it has inlined data?
			add			esi, 1024					; FSZ_Inode.inlinedata
			cmp			dword [esi], 'FSDR'
			je			.srchsbin
			; no, locate the data
			mov			eax, dword [esi-1024+448]	; FSZ_Inode.sec
			shl			eax, 12
			mov			esi, dword [bootboot.initrd_ptr]
			add			esi, eax
			cmp			dword [esi], 'FSDR'
			jne			.noroot
.srchsbin:	; find sbin
			mov			ecx, dword [esi+8]			; FSZ_DirEntHeader.numentries
			mov			eax, 'lib/'					; don't allow other
@@:			add			esi, 128					; directories than
			cmp			dword [esi+17], eax			; lib/
			je			@f
			dec			ecx
			jnz			@b
			jmp			.nolib
			; found, get it's inode
@@:
			mov			eax, dword [esi]
			shl			eax, 12
			mov			esi, dword [bootboot.initrd_ptr]
			add			esi, eax
			cmp			dword [esi], 'FSIN'
			jne			.nolib
			; it has inlined data?
			add			esi, 1024					; FSZ_Inode.inlinedata
			cmp			dword [esi], 'FSDR'
			je			.srchcore
			; no, locate the data
			mov			eax, dword [esi-1024+448]	; FSZ_Inode.sec
			shl			eax, 12
			mov			esi, dword [bootboot.initrd_ptr]
			add			esi, eax
			cmp			dword [esi], 'FSDR'
			jne			.nolib
.srchcore:	; find core in lib/
			mov			ecx, dword [esi+8]			; FSZ_DirEntHeader.numentries
			;filename, 8 characters supported
			mov			eax, dword [kernel+4]
			mov			edx, dword [kernel+8]
@@:			add			esi, 128
			cmp			dword [esi+21], edx
			jne			.not
			cmp			dword [esi+17], eax
			je			@f
.not:		dec			ecx
			jnz			@b
			jmp			.nocore
			; found, get it's inode
@@:			mov			eax, dword [esi]
			shl			eax, 12
			mov			esi, dword [bootboot.initrd_ptr]
			add			esi, eax
			cmp			dword [esi], 'FSIN'
			jne			.nocore
			; get data
			mov			eax, dword [esi+448]	; FSZ_Inode.sec
			mov			ecx, dword [esi+464]	; FSZ_Inode.size
			shl			eax, 12
			mov			esi, dword [bootboot.initrd_ptr]
			add			esi, eax

			; core's sector directory at esi, file size in ecx
			mov			eax, dword [esi]	; first FSZ_SectorList.sec
			shl			eax, 12
			mov			esi, dword [bootboot.initrd_ptr]
			add			esi, eax
			ret

; ----------- cpio ----------
cpio_1stpart_initrd:
			mov			ebx, ecx
			xor			ecx, ecx
			mov			eax, '0707'
@@:			add			esi, 512
			cmp			dword [esi], eax	; cpio magic
			jne			.not
			cmp			word [esi+4], ax	; cpio magic
			je			@f
.not:		cmp			esi, ebx
			jb			@b
			xor			ecx, ecx
			ret
			; get total number of sectors
@@:			mov			ecx, 2*1024*1024
			ret

; Find the ELF kernel on initrd
; IN:   esi: initrd pointer, ecx: initrd end, edi: kernel filename
; OUT:  On Success
;         esi: pointer to an ELF64 executable, ecx: size in bytes
;       On Error
;         esi: error message, ecx: 0
cpio_initrd:
			; upper bound
			mov			ebx, ecx
			xor			ecx, ecx
			; strlen(kernel)
			mov			eax, edi
			or			eax, eax
			jz			.err
			cmp			byte [eax], 0
			jz			.err
			xor			ecx, ecx
@@:			inc			ecx
			inc			eax
			cmp			byte [eax], 0
			jnz			@b
			mov			dword [.ks], ecx
			; while(ptr.magic=='070707' && ptr<limit)
.next:		cmp			esi, ebx
			jae			.err
			mov			eax, '0707'
			cmp			dword [esi], eax	; cpio magic
			jne			.err
			cmp			word [esi+4], ax	; cpio magic
			je			@f
.err:		xor			ecx, ecx
			ret
@@:			mov			eax, esi			; filename len
			add			eax, 8*6+11
			mov			ecx, 6
			call		prot_oct2bin
			mov			dword [.ns], eax
			mov			eax, esi			; filesize
			add			eax, 8*6+11+6
			mov			ecx, 11
			call		prot_oct2bin
			mov			dword [.fs], eax
			push		esi					; name equals?
			push		edi
			add			esi, 9*6+2*11
			mov			ecx, dword [.ks]
			repz		cmpsb
			pop			edi
			pop			esi
			jz			@f
			add			esi, 76				; no skip this record
			add			esi, dword [.ns]	; and check the next one
			add			esi, dword [.fs]
			jmp			.next
@@:			add			esi, 76				; found! esi=data
			add			esi, dword [.ns]
			mov			ecx, dword [.fs]	; ecx=size
			ret
.ks:		dd			0
.ns:		dd			0
.fs:		dd			0

; ----------- tar ----------
tar_1stpart_initrd:
			mov			ebx, ecx
			xor			ecx, ecx
			mov			eax, 'usta'
@@:			add			esi, 512
			cmp			dword [esi+257], eax	; tar magic
			jne			.not
			cmp			byte [esi+261], 'r'		; tar magic
			je			@f
.not:		cmp			esi, ebx
			jb			@b
			xor			ecx, ecx
			ret
			; get total number of sectors
@@:			mov			ecx, 2*1024*1024
			ret

; Find the ELF kernel on initrd
; IN:   esi: initrd pointer, ecx: initrd end, edi: kernel filename
; OUT:  On Success
;         esi: pointer to an ELF64 executable, ecx: size in bytes
;       On Error
;         esi: error message, ecx: 0
tar_initrd:
			; upper bound
			mov			ebx, ecx
			xor			ecx, ecx
			; strlen(kernel)
			mov			eax, edi
			or			eax, eax
			jz			.err
			cmp			byte [eax], 0
			jz			.err
			xor			ecx, ecx
@@:			inc			ecx
			inc			eax
			cmp			byte [eax], 0
			jnz			@b
			mov			dword [.ks], ecx
			; while(ptr.magic=='ustar' && ptr<limit)
.next:		cmp			esi, ebx
			jae			.err
			cmp			dword [esi+257], 'usta'	; tar magic
			jne			.err
			cmp			byte [esi+261], 'r'		; tar magic
			je			@f
.err:		xor			ecx, ecx
			ret
@@:			mov			eax, esi			; filesize
			add			eax, 07ch
			mov			ecx, 11
			call		prot_oct2bin
			mov			dword [.fs], eax
			push		esi					; name equals?
			push		edi
			mov			ecx, dword [.ks]
			repz		cmpsb
			pop			edi
			pop			esi
			jz			@f
			add			esi, 512			; no skip this record
			mov			eax, dword [.fs]	; and check the next one
			add			eax, 511
			shr			eax, 9
			shl			eax, 9
			add			esi, eax
			jmp			.next
@@:			add			esi, 512			; found! esi=data
			mov			ecx, dword [.fs]	; ecx=size
			ret
.ks:		dd			0
.fs:		dd			0

; ----------- FAT12 ----------
fat12_1stpart_initrd:
			mov			ebx, esi
			add			ebx, ecx
			mov			eax, 'FAT1'
@@:			add			esi, 512
			cmp			dword [esi+036h], eax	; FAT12 magic
			jne			.not
			cmp			byte [esi+03Ah], '2'	; FAT12 magic
			je			@f
.not:		cmp			esi, ebx
			jb			@b
			xor			ecx, ecx
			ret
			; get total number of sectors
@@:			mov			ecx, dword [esi+020h]
			or			ecx, ecx
			jnz			@f
			xor			ecx, ecx
			mov			cx, word [esi+013h]
			; sec*512
			shl			ecx, 9
			ret

fat12_initrd:
			cmp			dword [esi+036h], 'FAT1'	; FAT12 magic
			jne			.not
			cmp			byte [esi+03Ah], '2'		; FAT12 magic
			je			@f
.not:		xor			ecx, ecx
			ret
@@:			mov			ebx, esi
			add			ebx, ecx
			xor			ecx, ecx
			ret

; ----------- FAT16 ----------
fat16_1stpart_initrd:
			mov			ebx, esi
			add			ebx, ecx
			mov			eax, 'FAT1'
@@:			add			esi, 512
			cmp			dword [esi+036h], eax	; FAT16 magic
			jne			.not
			cmp			byte [esi+03Ah], '6'	; FAT16 magic
			je			@f
.not:		cmp			esi, ebx
			jb			@b
			xor			ecx, ecx
			ret
			; get total number of sectors
@@:			mov			ecx, dword [esi+020h]
			or			ecx, ecx
			jnz			@f
			xor			ecx, ecx
			mov			cx, word [esi+013h]
			; sec*512
			shl			ecx, 9
			ret

fat16_initrd:
			cmp			dword [esi+036h], 'FAT1'	; FAT16 magic
			jne			.not
			cmp			byte [esi+03Ah], '6'		; FAT16 magic
			je			@f
.not:		xor			ecx, ecx
			ret
@@:			mov			ebx, esi
			add			ebx, ecx
			xor			ecx, ecx
			ret

gzip_1stpart_initrd:
			mov			ebx, ecx
			xor			ecx, ecx
@@:			add			esi, 512
			cmp			word [esi], 08B1Fh			; gzip magic
			jne			.not
			cmp			dword [esi+10], 'init'		; original filename
			jne			.not
			cmp			word [esi+14], 'rd'
			jne			.not
			cmp			byte [esi+16], 0
			je			@f
.not:		cmp			esi, ebx
			jb			@b
			xor			ecx, ecx
			ret
			; get total number of sectors
@@:			mov			ecx, 2*1024*1024
			ret

gzip_initrd:
			cmp			word [esi], 08B1Fh			; gzip magic
			je			@f
			xor			ecx, ecx
			ret
@@:			mov			esi, nogzip
			jmp			prot_diefunc
