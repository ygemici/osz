#include <osZ.h>
#include <syscall.h>

void exit(int code)
{
    /* TODO: call atexit handlers and dtors */
    mq_call(SRV_core, SYS_exit, code, 0, 0);
    while(1);
}

