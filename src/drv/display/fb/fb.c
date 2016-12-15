#include <osZ.h>

extern void register_irq(uint8_t irq, void *handler);

void valami()
{
}

void _init()
{
	register_irq(1, valami);
}
