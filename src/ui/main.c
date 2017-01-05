#include <osZ.h>

extern char _binary_logo_start;

public void keypress(){}
public void keyrelease(){}

public void opentty(){}
public void openwin(){}
public void openwrd(){}

void _init()
{
    uint64_t len;
    /* map keymap */
    len = mapfile((void*)BSS_ADDRESS, "/etc/kbd/en");
    len++;
    mq_dispatch();
}
