;*
;* loader/mb-x86_64/fs.inc
;*
;* Copyright 2017 Public Domain BOOTBOOT bztsrc@github
;*
;* This file is part of the BOOTBOOT Protocol package.
;* @brief Filesystem drivers for initial ramdisk.
;*

FSZ_SUPPORT equ 1

;*********************************************************************
;*                       File System Drivers                         *
;*********************************************************************

            USE32
fsdrivers:
if FSZ_SUPPORT eq 1
            dw          fsz_initrd
end if
            dw          cpio_initrd
            dw          tar_initrd
            dw          0

if FSZ_SUPPORT eq 1
; ----------- FS/Z ----------
; Find the ELF kernel on initrd
; IN:   esi: initrd pointer, ecx: initrd end, edi: kernel filename
; OUT:  On Success
;         esi: pointer to an ELF64 executable, ecx: size in bytes
;       On Error
;         esi: error message, ecx: 0
fsz_initrd:
            mov         ebx, ecx
            xor         ecx, ecx
            ; FS/Z superblock
            ; get root dir inode
            mov         eax, dword [esi+552]    ; FSZ_SuperBlock.rootdirfid
            shl         eax, 12
            add         esi, eax
            cmp         dword [esi], 'FSIN'
            je          @f
.nolib:     mov         esi, nolib
.err:       xor         ecx, ecx
            ret
.nocore:    mov         esi, nocore
            jmp         .err
@@:         ; it has inlined data?
            add         esi, 1024                   ; FSZ_Inode.inlinedata
            cmp         dword [esi], 'FSDR'
            je          .srchlib
            ; no, locate the data
            mov         eax, dword [esi-1024+448]   ; FSZ_Inode.sec
            shl         eax, 12
            mov         esi, dword [bootboot.initrd_ptr]
            add         esi, eax
            cmp         dword [esi], 'FSDR'
            jne         .nolib
.srchlib:   ; find lib/
            mov         ecx, dword [esi+8]          ; FSZ_DirEntHeader.numentries
            mov         eax, 'lib/'                 ; don't allow other
@@:         add         esi, 128                    ; directories than
            cmp         dword [esi+17], eax         ; lib/
            je          @f
            dec         ecx
            jnz         @b
            jmp         .nolib
            ; found, get it's inode
@@:
            mov         eax, dword [esi]
            shl         eax, 12
            mov         esi, dword [bootboot.initrd_ptr]
            add         esi, eax
            cmp         dword [esi], 'FSIN'
            jne         .nolib
            ; it has inlined data?
            add         esi, 1024                   ; FSZ_Inode.inlinedata
            cmp         dword [esi], 'FSDR'
            je          .srchsys
            ; no, locate the data
            mov         eax, dword [esi-1024+448]   ; FSZ_Inode.sec
            shl         eax, 12
            mov         esi, dword [bootboot.initrd_ptr]
            add         esi, eax
            cmp         dword [esi], 'FSDR'
            jne         .nolib

.srchsys:   ; find sys in sys/
            mov         ecx, dword [esi+8]          ; FSZ_DirEntHeader.numentries
            mov         eax, 'sys/'
@@:         add         esi, 128
            cmp         dword [esi+17], eax
            je          @f
            dec         ecx
            jnz         @b
            jmp         .nolib
            ; found, get it's inode
@@:
            mov         eax, dword [esi]
            shl         eax, 12
            mov         esi, dword [bootboot.initrd_ptr]
            add         esi, eax
            cmp         dword [esi], 'FSIN'
            jne         .nolib
            ; it has inlined data?
            add         esi, 1024                   ; FSZ_Inode.inlinedata
            cmp         dword [esi], 'FSDR'
            je          .srchcore
            ; no, locate the data
            mov         eax, dword [esi-1024+448]   ; FSZ_Inode.sec
            shl         eax, 12
            mov         esi, dword [bootboot.initrd_ptr]
            add         esi, eax
            cmp         dword [esi], 'FSDR'
            jne         .nolib

.srchcore:  ; find core in lib/sys. This filename part is configurable
            mov         ecx, dword [esi+8]          ; FSZ_DirEntHeader.numentries
            ;filename, 8 characters supported
            mov         eax, dword [kernel+8]
            mov         edx, dword [kernel+12]
@@:         add         esi, 128
            cmp         dword [esi+21], edx
            jne         .not
            cmp         dword [esi+17], eax
            je          @f
.not:       dec         ecx
            jnz         @b
            jmp         .nocore
            ; found, get it's inode
@@:         mov         eax, dword [esi]
            shl         eax, 12
            mov         esi, dword [bootboot.initrd_ptr]
            add         esi, eax
            cmp         dword [esi], 'FSIN'
            jne         .nocore
            ; get data
            mov         eax, dword [esi+448]    ; FSZ_Inode.sec
            mov         ecx, dword [esi+464]    ; FSZ_Inode.size
            shl         eax, 12
            mov         esi, dword [bootboot.initrd_ptr]
            add         esi, eax

            ; core's sector directory at esi, file size in ecx
            mov         eax, dword [esi]    ; first FSZ_SectorList.sec
            shl         eax, 12
            mov         esi, dword [bootboot.initrd_ptr]
            add         esi, eax
            ret
end if

; ----------- cpio ----------
; Find the ELF kernel on initrd
; IN:   esi: initrd pointer, ecx: initrd end, edi: kernel filename
; OUT:  On Success
;         esi: pointer to an ELF64 executable, ecx: size in bytes
;       On Error
;         esi: error message, ecx: 0
cpio_initrd:
            ; upper bound
            mov         ebx, ecx
            xor         ecx, ecx
            ; strlen(kernel)
            mov         eax, edi
            or          eax, eax
            jz          .err
            cmp         byte [eax], 0
            jz          .err
            xor         ecx, ecx
@@:         inc         ecx
            inc         eax
            cmp         byte [eax], 0
            jnz         @b
            mov         dword [.ks], ecx
            ; while(ptr.magic=='070707' && ptr<limit)
.next:      cmp         esi, ebx
            jae         .err
            mov         eax, '0707'
            cmp         dword [esi], eax    ; cpio magic
            jne         .err
            cmp         word [esi+4], ax    ; cpio magic
            je          @f
.err:       xor         ecx, ecx
            ret
@@:         mov         eax, esi            ; filename len
            add         eax, 8*6+11
            mov         ecx, 6
            call        prot_oct2bin
            mov         dword [.ns], eax
            mov         eax, esi            ; filesize
            add         eax, 8*6+11+6
            mov         ecx, 11
            call        prot_oct2bin
            mov         dword [.fs], eax
            push        esi                 ; name equals?
            push        edi
            add         esi, 9*6+2*11
            mov         ecx, dword [.ks]
            repz        cmpsb
            pop         edi
            pop         esi
            jz          @f
            add         esi, 76             ; no skip this record
            add         esi, dword [.ns]    ; and check the next one
            add         esi, dword [.fs]
            jmp         .next
@@:         add         esi, 76             ; found! esi=data
            add         esi, dword [.ns]
            mov         ecx, dword [.fs]    ; ecx=size
            ret
.ks:        dd          0
.ns:        dd          0
.fs:        dd          0

; ----------- tar ----------
; Find the ELF kernel on initrd
; IN:   esi: initrd pointer, ecx: initrd end, edi: kernel filename
; OUT:  On Success
;         esi: pointer to an ELF64 executable, ecx: size in bytes
;       On Error
;         esi: error message, ecx: 0
tar_initrd:
            ; upper bound
            mov         ebx, ecx
            xor         ecx, ecx
            ; strlen(kernel)
            mov         eax, edi
            or          eax, eax
            jz          .err
            cmp         byte [eax], 0
            jz          .err
            xor         ecx, ecx
@@:         inc         ecx
            inc         eax
            cmp         byte [eax], 0
            jnz         @b
            mov         dword [.ks], ecx
            ; while(ptr.magic=='ustar' && ptr<limit)
.next:      cmp         esi, ebx
            jae         .err
            cmp         dword [esi+257], 'usta' ; tar magic
            jne         .err
            cmp         byte [esi+261], 'r'     ; tar magic
            je          @f
.err:       xor         ecx, ecx
            ret
@@:         mov         eax, esi            ; filesize
            add         eax, 07ch
            mov         ecx, 11
            call        prot_oct2bin
            mov         dword [.fs], eax
            push        esi                 ; name equals?
            push        edi
            mov         ecx, dword [.ks]
            repz        cmpsb
            pop         edi
            pop         esi
            jz          @f
            add         esi, 512            ; no skip this record
            mov         eax, dword [.fs]    ; and check the next one
            add         eax, 511
            shr         eax, 9
            shl         eax, 9
            add         esi, eax
            jmp         .next
@@:         add         esi, 512            ; found! esi=data
            mov         ecx, dword [.fs]    ; ecx=size
            ret
.ks:        dd          0
.fs:        dd          0
