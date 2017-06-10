#include <osZ.h>
#include <sys/sysinfo.h>

public void start(){}
public void stop(){}
public void restart(){}
public void status(){}

void _init()
{
    sysinfo_t *sysi;
    //wait for sys_ready() to send an SYS_ack
    mq_recv();

    mq_recv();
    sysi = sysinfo();
    if(sysi->rescueshell) {
        breakpoint;
    } else {
ee:goto ee;
    }
}
