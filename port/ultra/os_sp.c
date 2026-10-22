/* Dr. Mario 64 -> Wii native port: SP task stub (task 07).
 * osSpTaskStart in sptask.h is a macro that calls osSpTaskLoad then
 * osSpTaskStartGo, so we implement those two. No real RSP/RDP work: gfx
 * tasks are logged, then the SP and DP "done" event messages are sent so
 * the scheduler's rspMQ/rdpMQ waits complete and the game loop advances. */

#include <PR/ultratypes.h>
#include <PR/sptask.h>
#include <PR/mbi.h>
#include <PR/os_message.h>
#include "port_log.h"
#include "port_audio.h"

extern void port_os_event(int event);
extern void gfx_run_task(OSTask *tp);
extern void rsp_enqueue(int type, OSTask *tp, void *fb);

static u32 gfxTaskCount = 0;

void osSpTaskLoad(OSTask *tp) {
    (void)tp; /* nothing to load; the work happens in osSpTaskStartGo */
}

void osSpTaskStartGo(OSTask *tp) {
    switch (tp->t.type) {
        case M_GFXTASK:
            gfxTaskCount++;
            if (gfxTaskCount <= 5)
                PORT_LOG("gfx task %d dl=%p", (int)gfxTaskCount, (void *)tp->t.data_ptr);
#ifdef PORT_LOG_VERBOSE
            else if ((gfxTaskCount % 60) == 0)
                PORT_LOG("gfx task %d dl=%p", (int)gfxTaskCount, (void *)tp->t.data_ptr);
#endif
            rsp_enqueue(0, tp, NULL);   /* RSP LWP runs gfx_run_task + posts SP/DP */
            break;
        case M_AUDTASK:
            /* task 15c2: audio must NOT go through the RSP LWP; run it
             * synchronously here and post OS_EVENT_SP as before. */
            { static int s_a = 0; if (!s_a) { s_a = 1; PORT_LOG("audio task via osSpTaskStart"); } }
            port_audio_task_run(tp);
            port_os_event(OS_EVENT_SP);
            break;
        default:
            port_os_event(OS_EVENT_SP);
            break;
    }
}

void osSpTaskYield(void) {
    /* no real SP: tasks never yield */
}

OSYieldResult osSpTaskYielded(OSTask *tp) {
    (void)tp;
    return 0; /* never yielded */
}
