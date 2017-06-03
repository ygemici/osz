;*
;* loader/mb-x86_64/tinf.inc
;*
;* Copyright 2017 Public Domain BOOTBOOT bztsrc@github
;*
;* This file is part of the BOOTBOOT Protocol package.
;* @brief Tiny inflate, ported after tinflate.c by bzt
;*

;IN:
; esi: gzipped initrd
; edi: output buffer (guaranteed to be big enough)
; ecx: output buffer size
tinf_uncompress:
            mov         dword [d_destSize], ecx
xchg bx,bx
.again:
            ; start a new block
            cmp         byte [d_btype], 255
            jne         .notst
            ; read final block flag
.next_blk:  call        tinf_getbit
            mov         byte [d_bfinal], al
            ; read block type
            xor         ebx, ebx
            mov         cl, 2
            call        tinf_read_bits
            mov         byte [d_btype], al
            ; build fixed huffman trees
            cmp         al, 1
            jne         @f
            call        tinf_build_fixed_trees
            xor         al, al
            ; decode trees from stream
@@:         cmp         al, 2
            jne         @f
            call        tinf_decode_trees
@@:
.notst:
            ; process current block
            cmp         byte [d_btype], 0
            jnz         @f
            ; decompress uncompressed block
            call        tinf_inflate_uncompressed_block
            JMP         .procend
@@:         cmp         byte [d_btype], 1
            je          @f
            cmp         byte [d_btype], 2
            jne         tinf_err
            ; decompress block with fixed/dyanamic huffman trees
            ; trees were decoded previously, so it's the same routine for both
@@:         call        tinf_inflate_block_data
.procend:   cmp         al, 1
            jne         @f
            cmp         byte [d_bfinal], 0
            ; the block has ended (without producing more data), but we
            ; can't return without data, so start procesing next block
            jz          .next_blk
            or          al, al
            jnz         tinf_err
            dec         dword [d_destSize]
            jnz         .again
            ret

; build the fixed huffman trees
tinf_build_fixed_trees:
            push        edi
            xor         ecx, ecx
            xor         eax, eax
            ; build fixed length tree
            mov         cl, 7
            mov         edi, d_ltree_table
            repnz       stosw
            mov         al, 24
            stosw
            mov         al, 152
            stosw
            mov         al, 112
            stosw
            mov         edi, d_ltree_trans

            mov         cl, 24
            mov         ax, 256
@@:         stosw
            inc         ax
            dec         cl
            jnz         @b

            mov         cl, 144
            xor         ax, ax
@@:         stosw
            inc         ax
            dec         cl
            jnz         @b

            mov         cl, 8
            mov         ax, 280
@@:         stosw
            inc         ax
            dec         cl
            jnz         @b

            mov         cl, 112
            mov         ax, 144
@@:         stosw
            inc         ax
            dec         cl
            jnz         @b

            ; build fixed distance tree
            mov         cl, 5
            mov         edi, d_dtree_table
            xor         ax, ax
            repnz       stosw
            mov         al, 32
            stosw
            
            mov         edi, d_dtree_trans
            mov         cl, 32
            xor         ax, ax
@@:         stosw
            inc         ax
            dec         cl
            jnz         @b

            pop         edi
            ret

;IN:
; ebp: TINF_TREE
; ebx: lengths
; ecx: num
tinf_build_tree:
            push        edi
            push        ecx
            xor         ecx, ecx
            xor         eax, eax
            ; clear code length count table
            mov         cl, 16
            mov         edi, ebp
            repnz       stosw
            mov         ecx, dword [esp]

            ; scan symbol lengths, and sum code length counts
            push        ebx
@@:         mov         ax, word [ebx]
            add         ebx, 2
            inc         word [ebp+eax]
            dec         cx
            jnz         @b
            mov         word [ebp], 0

            mov         cl, 16
            mov         edx, offs
            mov         ebx, ebp    ;table
            xor         eax, eax    ;sum
@@:         mov         word [edx], ax
            add         ax, word [ebx]
            add         ebx, 2
            add         edx, 2
            dec         cx
            jnz         @b

            pop         ebx         ;lengths
            pop         ecx         ;num

            ; create code->symbol translation table (symbols sorted by code)
            xor         eax, eax    ;i
            xor         edx, edx
@@:         cmp         word [ebx], 0
            jz          .null
            mov         dx, word [ebx+2*eax]    ;lengths[i]
            shl         edx, 1
            add         edx, offs               ;offs[lengths[i]]
            mov         word [ebp+2*edx+32], ax
            inc         word [edx]
.null:      add         ebx, 2
            inc         eax
            dec         cx
            jnz         @b

            pop         edi
            ret

tinf_decode_trees:
            mov         word [num], 0

            ; get 5 bits HLIT (257-286)
            xor         ecx, ecx
            mov         cl, 5
            mov         ebx, 257
            call        tinf_read_bits
            mov         byte [hlit], al
            mov         byte [num], al

            ; get 5 bits HDIST (1-32)
            mov         cl, 5
            xor         ebx, ebx
            inc         ebx
            call        tinf_read_bits
            mov         byte [hdist], al
            mov         word [num], ax

            ; get 4 bits HCLEN (4-19)
            mov         cl, 4
            mov         ebx, ecx
            call        tinf_read_bits
            mov         byte [hclen], al

            push        edi
            mov         cl, 19
            mov         edi, lengths
            xor         ax, ax
            repnz       stosw
            pop         edi

            ; read code lengths for code length alphabet
            mov         edx, clcidx
            ; get 3 bits code length (0-7)
@@:         mov         cl, 3
            xor         ebx, ebx
            call        tinf_read_bits
            mov         bx, word [edx]  ;clcidx[i]
            mov         byte[ebx+lengths], al
            add         edx, 2
            dec         byte [hclen]
            jnz         @b

            ; build code length tree, temporarily use length tree
            mov         ebp, d_ltree
            mov         ebx, lengths
            xor         ecx, ecx
            mov         cl, 19
            call        tinf_build_tree
            
            ; decode code lengths for the dynamic trees
            mov         edx, lengths
.decode:    mov         ebp, d_ltree
            call        tinf_decode_symbol
            xor         ebx, ebx
            cmp         al, 16
            jne         .not16
            ; copy previous code length 3-6 times (read 2 bits)
            mov         cl, 2
            mov         bl, 3
            call        tinf_read_bits
            mov         cl, al
            mov         al, byte [edx-2]    ;lengths[num-1]
@@:         mov         byte [edx], al
            add         edx, 2
            dec         word [num]
            dec         cl
            jnz         @b
            jmp         .next

.not16:     cmp         al, 17
            jne         .not17
            ; repeat code length 0 for 3-10 times (read 3 bits)
            mov         cl, 3
            mov         bl, cl
            call        tinf_read_bits
            mov         cl, al
@@:         mov         byte [edx], 0
            add         edx, 2
            dec         word [num]
            dec         cl
            jnz         @b
            jmp         .next

.not17:     cmp         al, 18
            jne         .not18
            ; repeat code length 0 for 11-138 times (read 7 bits)
            mov         cl, 7
            mov         bl, 11
            call        tinf_read_bits
            mov         cl, al
@@:         mov         byte [edx], 0
            add         edx, 2
            dec         word [num]
            dec         cl
            jnz         @b
            jmp         .next

.not18:     ; values 0-15 represent the actual code lengths
            mov         byte [edx], al
            add         edx, 2
            dec         word [num]
      
.next:      cmp         word [num], 0
            jnz         .decode

            ; build dynamic trees
            mov         ebp, d_ltree
            mov         ebx, lengths
            xor         ecx, ecx
            mov         cl, byte [hlit]
            call        tinf_build_tree

            mov         ebp, d_dtree
            mov         ebx, lengths
            xor         ecx, ecx
            mov         cl, byte [hlit]
            add         ebx, ecx
            mov         cl, byte [hdist]
            call        tinf_build_tree
            ret

;OUT:
; al: status
tinf_inflate_block_data:
            cmp         word [d_curlen], 0
            jne         .notnull
            mov         ebp, d_ltree
            call        tinf_decode_symbol

            ; literal byte
            cmp         ax, 256
            jae         @f
            call        tinf_put
            xor         al, al
            ret
@@:         cmp         ax, 256
            jne         @f
            ; end of block
            mov         al, 1
            ret
@@:         ; substring from sliding dictionary
            sub         eax, 257
            ; possibly get more bits from length code
            xor         ecx, ecx
            mov         cl, byte [length_bits+eax]
            xor         ebx, ebx
            mov         bx, word [length_base+2*eax]
            call        tinf_read_bits
            mov         word [d_curlen], ax
            ; possibly get more bits from distance code
            mov         ebp, d_dtree
            call        tinf_decode_symbol
            xor         ecx, ecx
            mov         cl, byte [dist_bits+eax]
            xor         ebx, ebx
            mov         bx, word [dists_base+2*eax]
            call        tinf_read_bits
            cmp         dword [d_dict_ring], 0
            jz          .noring1
            cmp         eax, dword [d_dict_size]
            ja          tinf_err
            mov         ebx, dword [d_dict_idx]
            sub         ebx, eax
            jns         @f
            add         ebx, dword [d_dict_size]
@@:         mov         dword [d_lzOff], ebx
            jmp         .notnull
.noring1:   neg         eax
            mov         dword [d_lzOff], eax
.notnull:

            ; copy next byte from dict substring
            cmp         dword [d_dict_ring], 0
            jz          .noring2
            mov         eax, dword [d_lzOff]
            add         eax, dword [d_dict_ring]
            call        tinf_put
            inc         dword [d_lzOff]
            mov         eax, dword [d_lzOff]
            cmp         eax, dword [d_dict_size]
            jne         @f
            mov         dword [d_lzOff], 0
            jmp         @f
.noring2:   mov         eax, edi
            add         eax, dword [d_lzOff]
            mov         al, byte [eax]
            stosb
@@:         dec         word [d_curlen]
            ret

;OUT:
; al: status
tinf_inflate_uncompressed_block:
            cmp         word [d_curlen], 0
            jne         @f
            ; get length
            lodsw
            ; get one's complement of length
            add         esi, 2
            ; increment length to properly return TINF_DONE below, without
            ; producing data at the same time
            mov         word [d_curlen], ax
            inc         word [d_curlen]
            ; make sure we start next block on a byte boundary
            mov         byte [d_bitcount], 0
@@:         dec         byte [d_curlen]
            cmp         byte [d_curlen], 0
            jnz         @f
            mov         al, 1
            ret
@@:         lodsb
            call        tinf_put
            xor         al, al
            ret

;OUT:
; al,carryflag bit
tinf_getbit:
            mov         al, byte [d_bitcount]
            or          al, al
            jnz         @f
            lodsb
            mov         byte [d_tag], al
            mov         byte [d_bitcount], 8
@@:         dec         byte [d_bitcount]
            xor         ax, ax
            shr         byte [d_tag], 1
            adc         ax, 0
            ret

;IN:
; ebx: base
; cl: num
;OUT:
; eax: bits
tinf_read_bits:
            push        edx
            or          cl, cl
            jnz         @f
            mov         eax, ebx
            ret
@@:         xor         eax, eax
            xor         edx, edx
            inc         dl
.next:      call        tinf_getbit
            jnc         @f
            add         eax, edx
@@:         shl         edx, 1
            dec         cl
            jnz         .next
            add         eax, ebx
            pop         edx
            ret

;IN:
; ebp: TINF_TREE
;OUT:
; eax: trans
tinf_decode_symbol:
            push        edx
            xor         eax, eax
            xor         ebx, ebx ;cur
            xor         ecx, ecx ;len
            xor         edx, edx ;sum
            ; get more bits while code value is above sum
@@:         shl         ebx, 1
            call        tinf_getbit
            add         ebx, eax
            inc         ecx
            mov         ax, word [ebp+2*ecx]
            add         edx, eax
            sub         ebx, eax
            jns         @b
            add         edx, ebx
            mov         ax, word [ebp+2*edx+32]
            pop         edx
            ret

;IN:
; al: char
tinf_put:
            push        ebx
            stosb
            cmp         dword [d_dict_ring], 0
            jz          @f
            mov         ebx, dword [d_dict_idx]
            add         ebx, dword [d_dict_ring]
            mov         byte [ebx], al
            inc         dword [d_dict_idx]
            mov         ebx, dword [d_dict_size]
            cmp         ebx, dword [d_dict_idx]
            jne         @f
            mov         dword [d_dict_idx], 0
@@:         pop         ebx
            ret

tinf_err:
            mov         esi, nogzip
            jmp         prot_diefunc

length_bits:
            db          0, 0, 0, 0, 0, 0, 0, 0
            db          1, 1, 1, 1, 2, 2, 2, 2
            db          3, 3, 3, 3, 4, 4, 4, 4
            db          5, 5, 5, 5
length_base:
            dw          3, 4, 5, 6, 7, 8, 9, 10
            dw          11, 13, 15, 17, 19, 23, 27, 31
            dw          35, 43, 51, 59, 67, 83, 99, 115
            dw          131, 163, 195, 227, 258
dist_bits:
            db          0, 0, 0, 0, 1, 1, 2, 2
            db          3, 3, 4, 4, 5, 5, 6, 6
            db          7, 7, 8, 8, 9, 9, 10, 10
            db          11, 11, 12, 12, 13, 13
dists_base:
            dw          1, 2, 3, 4, 5, 7, 9, 13
            dw          17, 25, 33, 49, 65, 97, 129, 193
            dw          257, 385, 513, 769, 1025, 1537, 2049, 3073
            dw          4097, 6145, 8193, 12289, 16385, 24577
clcidx:
            db          16, 17, 18, 0, 8, 7, 9, 6
            db          10, 5, 11, 4, 12, 3, 13, 2
            db          14, 1, 15

d_destSize: dd          0
d_lzOff:    dd          0
d_dict_ring:dd          0
d_dict_size:dd          0
d_dict_idx: dd          0
d_tag:      db          0
d_bitcount: db          0
d_btype:    db          255
d_bfinal:   db          0
d_curlen:   dw          0
d_ltree:
d_ltree_table:
            dw          16 dup 0
d_ltree_trans:
            dw          288 dup 0
d_dtree:
d_dtree_table:
            dw          16 dup 0
d_dtree_trans:
            dw          288 dup 0
offs:       dw          16 dup 0
num:        dw          0
lengths:    db          288+32 dup 0
hlit:       db          0
hdist:      db          0
hclen:      db          0
