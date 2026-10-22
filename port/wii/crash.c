/* Crash hook: on a CPU exception, save the fault details and redirect the faulting thread to
 * port_crash_report(), which logs them through the normal SD log and then parks the thread. */
#include <gccore.h>
#include <unistd.h>
#include <tuxedo/ppc/exception.h>
#include <tuxedo/ppc/context.h>
#include "port_log.h"
static volatile u32 s_ex, s_pc, s_lr, s_dar, s_dsisr, s_r1, s_r3, s_r4;
static void port_crash_report(void)
{
    PORT_LOG("CRASH ex=%u pc=%08x lr=%08x dar=%08x dsisr=%08x r1=%08x r3=%08x r4=%08x",
             s_ex, s_pc, s_lr, s_dar, s_dsisr, s_r1, s_r3, s_r4);
    port_log_flush();
    for (;;) usleep(1000000);
}
static void port_panic(unsigned exid, PPCContext *ctx)
{
    u32 dar, dsisr;
    __asm__ volatile("mfspr %0, 19" : "=r"(dar));
    __asm__ volatile("mfspr %0, 18" : "=r"(dsisr));
    s_ex = exid; s_pc = ctx->pc; s_lr = ctx->lr; s_dar = dar; s_dsisr = dsisr;
    s_r1 = ctx->gpr[1]; s_r3 = ctx->gpr[3]; s_r4 = ctx->gpr[4];
    ctx->pc = (u32)port_crash_report;
    ctx->msr |= 0x8000; /* EE */
}
void port_crash_install(void) { PPCExcptCurPanicFn = port_panic; }
