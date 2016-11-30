.section .text
.global isr_syscall

/* syscall dispatcher */
isr_syscall:
	sysret
