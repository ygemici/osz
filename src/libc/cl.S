# Coder's Language Dynamic Linkage information

.extern width
.extern printf

# Header
.section .cl
.byte 'C','L','D','L'
.hword 215, 0			# size

.hword 0x0003			# public property
.hword 139, 144, 153	# uint32_t libc.width
.long width			# entry point
.hword 159				# dec(min=40,max=132)
.hword 0x0000			# pointer mask
# Arguments
.hword 0, 0
.hword 0, 0
.hword 0, 0
.hword 0, 0
.hword 0, 0
.hword 0, 0
.hword 0, 0
.hword 0, 0
.hword 0, 0
.hword 0, 0
.hword 0, 0
.hword 0, 0

.hword 0x000a			# public static method
.hword 139, 179, 186	# uint32 libc.printf
.long printf			# entry point
.hword 0
.hword 0x0001			# pointer mask
# Arguments
.hword 193, 199			# *uint8 fmt
.hword 203, 138			# ... 
.hword 0, 0
.hword 0, 0
.hword 0, 0
.hword 0, 0
.hword 0, 0
.hword 0, 0
.hword 0, 0
.hword 0, 0
.hword 0, 0
.hword 0, 0

.byte 0xFF, 0xFF		# no more records

# String table
.asciz ""
.asciz "libc"
.asciz "uint32_t"
.asciz "width"
.asciz "dec(min=40,max=132)"
.asciz "uint32"
.asciz "printf"
.asciz "uint8"
.asciz "fmt"
.asciz "__VA_ARGS__"
.byte 0
