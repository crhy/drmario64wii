/* Dr. Mario 64 -> Wii native port: real IRQ disable/restore (task 12c item 1).
 * libogc's IRQ_Disable/IRQ_Restore are header-inline; wrapping them in a
 * non-inlined TU gives the event ring a single, real interrupt-guard that
 * port_post_event can call from any producer (VI IRQ, timer LWP, RSP LWP). */

#include <ogc/irq.h>

unsigned port_irq_disable(void)
{
    return (unsigned)IRQ_Disable();
}

void port_irq_restore(unsigned level)
{
    IRQ_Restore((unsigned int)level);
}
