#include <osZ.h>
#include <sys/sysinfo.h>

public uint64_t __attribute__ ((section (".data"))) tmrfreq = 1000000;

public void irq0()
{
    /* nothing to do, but we still need a stub function 
       to inform SYS task we need IRQ enabled */
}

void _init()
{
__asm__ __volatile__ ("xchg %%bx, %%bx" : : : );
    sysinfo_t *si = sysinfo();
dbg_printf("Hpet test %x\n", si);
}
