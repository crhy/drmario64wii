/* Debug sentinel: the VI IRQ callback samples the interrupted thread's PC/LR (tuxedo saves it in
 * KThread.ctx). Every 120 retraces a priority-127 LWP appends the samples to sd:/drmario/sentinel.txt
 * through its own FILE, so it still reports when the game, the event pump or PORT_LOG are stuck. */
#include <gccore.h>
#include <stdio.h>
#include <unistd.h>
#include <tuxedo/thread.h>
#ifdef PORT_SENTINEL
static volatile u32 s_pc[8], s_lr[8], s_n;
static sem_t s_sem; static lwp_t s_lwp;
static u8 s_stack[16384] __attribute__((aligned(32)));
void port_sentinel_sample(void)
{
    KThread *t = KThreadGetSelf();
    u32 i = s_n++ & 7;
    s_pc[i] = t ? t->ctx.pc : 0; s_lr[i] = t ? t->ctx.lr : 0;
    if ((s_n % 30) == 0) LWP_SemPost(s_sem);
}
static void *sentinel(void *a)
{
    (void)a;
    for (;;) {
        LWP_SemWait(s_sem);
        FILE *f = fopen("sd:/drmario/sentinel.txt", "a");
        if (!f) continue;
        fprintf(f, "n=%u", (unsigned)s_n);
        for (int i = 0; i < 8; i++) fprintf(f, " %08x/%08x", (unsigned)s_pc[i], (unsigned)s_lr[i]);
        fputc('\n', f); fclose(f);
    }
    return NULL;
}
void port_sentinel_start(void) { LWP_SemInit(&s_sem, 0, 1000); LWP_CreateThread(&s_lwp, sentinel, NULL, s_stack, sizeof s_stack, 127); }
#endif
