#include <osZ.h>

extern char _binary_logo_start;

public void keypress(){}
public void keyrelease(){}

public void opentty(){}
public void openwin(){}
public void openwrd(){}

void _init()
{
    uint64_t bss = BSS_ADDRESS;
    uint64_t len;
    /* map keymap */
asm("int $1":::);
    len = mapfile((void*)bss, "/etc/kbd/en_us");
    bss += (len + __PAGESIZE-1) & ~(__PAGESIZE-1);

    mq_dispatch();
}
