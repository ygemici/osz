.section .text
.global kmemcpy
.global kmemset
.global kmemvid
.global kmap
.global kmap_init
.extern kmap_tmp
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
	call	kmap_getpte
	andw	$0xF000, %si
	incw	%si
	movq	%rsi, (%rax)
	invlpg	(%rsi)
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
	pushq	%rdi
	mov		%rbx, %rax
	addq	$8, %rax
	movq	%rax, %rdi
	andw	$0x0F000, %ax
	incw	%ax
	mov		%rax, (%rbx)
	/* clear the rest */
	pushq	%rcx
	pushq	%rax
	xorq	%rax, %rax
	movq	%rbx, %rcx
	addq	$4095, %rcx
	andw	$0xF000, %cx
	subq	%rbx, %rcx
	shrq	$3, %rcx
	dec		%rcx
	dec		%rcx
	repnz	stosq
	popq	%rax
	popq	%rcx
	popq	%rdi
	/* record pointer */
	subq	$16, %rbx
	andq	$0x0FFF, %rbx
	addq	%rcx, %rbx
	movq	%rbx, %rax
	popq	%rcx
	popq	%rbx
	ret

kmap_getpte:
	/* we have at least tmpmap, so we can map PTE */
	movq	kmap_tmp, %rbx
	/* PML4 */
	movq	%rdi, %rax
	shrq	$12+9+9+9, %rax
	andl	$0x1FF, %eax
	shlq	$3, %rax
	movq	%cr3, %rdx
	andw	$0xF000, %dx
	incw	%dx
	movq	%rdx, (%rbx)
	invlpg	tmpmap
	addq	$tmpmap, %rax
	/* PDPE */
	mov		(%rax), %rdx
	movq	%rdi, %rax
	shrq	$12+9+9, %rax
	andl	$0x1FF, %eax
	shlq	$3, %rax
	andw	$0xF000, %dx
	incw	%dx
	movq	%rdx, (%rbx)
	invlpg	(%rbx)
	invlpg	tmpmap
	addq	$tmpmap, %rax
	/* PDE */
	mov		(%rax), %rdx
	movq	%rdi, %rax
	shrq	$12+9, %rax
	andl	$0x1FF, %eax
	shlq	$3, %rax
	andw	$0xF000, %dx
	incw	%dx
	movq	%rdx, (%rbx)
	invlpg	(%rbx)
	invlpg	tmpmap
	addq	$tmpmap, %rax
	/* PTE */
	mov		(%rax), %rdx
	movq	%rdi, %rax
	shrq	$12, %rax
	andl	$0x1FF, %eax
	shlq	$3, %rax
	andw	$0xF000, %dx
	incw	%dx
	movq	%rdx, (%rbx)
	invlpg	(%rbx)
	invlpg	tmpmap
	addq	$tmpmap, %rax
	ret
