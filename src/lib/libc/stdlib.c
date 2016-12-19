#include <osZ.h>
#include <syscall.h>

extern int clcall(int method, ...);

/* this has to be a C function, because stupid gcc does not
 * save size for asm functions, and complains about it... */
void exit(int code)
{
	clcall(SYS_exit, code);
	while(1);
}

