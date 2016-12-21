#include <osZ.h>
#include <syscall.h>

/* this has to be a C function, because stupid gcc does not
 * save size for asm functions, and complains about it... */
void exit(int code)
{
	clcall(SRV_core, SYS_exit, code, 0, 0);
	while(1);
}

