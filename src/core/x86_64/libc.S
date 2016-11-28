.section .text
.global kmemcpy
.global kmemset
.global kmemvid
.global kmap
.global kmap_tmp

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
kmap_tmp:
.quad 0
