.section .text
.global kmemcpy
.global kmemset
.global kmemvid
.global kmap
.extern tmpmap
.extern tmp2map

/* misc memory functions */
kmemcpy:
	movq	8(%rsp), %rdi
	movq	16(%rsp), %rsi
	movq	24(%rsp), %rcx
	repnz	movsb
	ret

kmemset:
	movq	8(%rsp), %rdi
	movq	16(%rsp), %rax
	movq	24(%rsp), %rcx
	repnz	stosb
	ret

kmemvid:
	ret

kmap:
	ret
