# Coder's Language Dynamic Linkage information

.extern printf

# Header
.section .cl
.byte 'C','L','D','L'
.hword 116, 0			# size

.hword 0x000a			# public static method
.hword 75, 80, 87	# uint32 libc.printf
.long printf			# entry point
.hword 0
.hword 0x0001			# pointer mask
# Arguments
.hword 94, 100			# *uint8 fmt
.hword 104, 74			# ... 
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
.asciz "uint32"
.asciz "printf"
.asciz "uint8"
.asciz "fmt"
.asciz "__VA_ARGS__"
.byte 0
