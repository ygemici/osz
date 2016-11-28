.section .text
.global isr_init
.global isr_syscall

isr_init:
	ret

/* syscall dispatcher */
isr_syscall:
	sysret
