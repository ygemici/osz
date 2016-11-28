.section .text
.global kmemcpy
.global kmemset
.global kmemvid
.global kmap
.global kmap_init
.extern tmpmap
.extern tmp2map
.extern tmppte

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

kmap_init:
	push	%rbx
	push	%rcx
	/* this is called very early. Relies on identity mapping
	   to find the physical address of tmpmap pointer in PTE */
	movq	$tmppte, %rcx
	/* PML4 */
	movq	%rcx, %rbx
	shrq	$12+9+9+9, %rbx
	andl	$0x1FF, %ebx
	shlq	$3, %rbx
	movq	%cr3, %rax
	add		%rax, %rbx
	/* PDPE */
	movq	(%rbx), %rax
	xorb	%al, %al
	movq	%rcx, %rbx
	shrq	$12+9+9, %rbx
	andl	$0x1FF, %ebx
	shlq	$3, %rbx
	add		%rax, %rbx
	/* PDE */
	movq	(%rbx), %rax
	xorb	%al, %al
	movq	%rcx, %rbx
	shrq	$12+9, %rbx
	andl	$0x1FF, %ebx
	shlq	$3, %rbx
	add		%rax, %rbx
	/* PTE */
	movq	(%rbx), %rax
	xorb	%al, %al
	movq	%rcx, %rbx
	shrq	$12, %rbx
	andl	$0x1FF, %ebx
	shlq	$3, %rbx
	add		%rax, %rbx
	/* map it at tmppte */
	mov		%rbx, %rax
	andw	$0x0F000, %ax
	incw	%ax
	mov		%rax, (%rbx)
	/* record pointer */
	subq	$16, %rbx
	andw	$0x0FFF, %bx
	addq	%rcx, %rbx
	mov		%rbx, %rax
	invlpg	(%rax)
	pop		%rcx
	pop		%rbx
	ret
