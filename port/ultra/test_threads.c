/* Dr. Mario 64 -> Wii native port: libultra thread + message-queue self-test.
 * Compiled only when WII_SELFTEST=1 (Makefile.wii flag). main() calls
 * port_selftest() INSTEAD of bootproc(), then blocks forever.
 *
 * T1 ping-pong 1000 messages between two pri-20 threads.
 * T2 NOBLOCK recv on empty returns -1; NOBLOCK send on full returns -1.
 * T3 preemption: low-pri L sends to a queue high-pri H waits on; H must run
 *    (increment a counter) before L's osSendMesg returns -> counter is 1.
 * T4 three threads A(10) B(20) C(20) each append a letter and osYieldThread()
 *    3 times, started from a starter S(50); expected order "BCBCBCAAA".
 * Each logs "T<n> ok" / "T<n> FAIL"; final "SELFTEST PASS" or "SELFTEST FAIL". */

#include <unistd.h>
#include <PR/ultratypes.h>
#include <PR/os_thread.h>
#include <PR/os_message.h>
#include <PR/os_time.h>
#include <PR/os_vi.h>
#include "port_log.h"

#define OS_CPU_COUNTER 46875000

/* wall-clock TBL ticks (60.75e6/s) provided by os_events.c for T7 */
extern unsigned long long port_wall_ticks_now(void);
#define WALL_CLOCK 60750000ULL

/* ---------- shared order log (T4) ---------- */
static char      g_order[64];
static int       g_order_len;

/* ---------- T1 ping-pong ---------- */
static OSMesgQueue t1_qab;
static OSMesg      t1_qab_buf[1];
static OSMesgQueue t1_qba;
static OSMesg      t1_qba_buf[1];
static int         t1_count;   /* messages received by A */
static int         t1_done;    /* set when A finishes its 1000 */

/* ---------- T3 preemption ---------- */
static OSMesgQueue t3_q;
static OSMesg      t3_q_buf[1];
static int         t3_counter; /* incremented by H */

/* ---------- the four T4 game threads ---------- */
static OSThread  th_a;
static OSThread  th_b;
static OSThread  th_c;
static OSThread  th_s;

/* ---------- T1 threads ---------- */
static OSThread  th_t1a;
static OSThread  th_t1b;

/* ---------- T3 threads ---------- */
static OSThread  th_t3h;
static OSThread  th_t3l;
static OSThread  th_t3s;

/* ---------- T5 VI events ---------- */
static OSMesgQueue t5_q;
static OSMesg      t5_q_buf[8];
static int         t5_count;   /* messages received by the pri-20 thread */
static int         t5_done;
static OSThread    th_t5;

/* ---------- T6 timers ---------- */
static OSMesgQueue t6_one_q;
static OSMesg      t6_one_buf[1];
static OSTimer     t6_one_timer;
static OSTime      t6_one_elapsed;
static int         t6_one_done;
static OSThread    th_t6one;

static OSMesgQueue t6_per_q;
static OSMesg      t6_per_buf[1];
static OSTimer     t6_per_timer;
static int         t6_per_count;
static int         t6_per_done;
static OSThread    th_t6per;

/* ---------- T7 osGetTime accuracy ---------- */
static OSTime      t7_delta;
static int         t7_done;
static OSThread    th_t7;

/* ===================== T4 bodies ===================== */
static void body_a(void *arg)
{
    int i;
    (void)arg;
    for (i = 0; i < 3; i++) {
        if (g_order_len < (int)sizeof(g_order) - 1)
            g_order[g_order_len++] = 'A';
        osYieldThread();
    }
}

static void body_b(void *arg)
{
    int i;
    (void)arg;
    for (i = 0; i < 3; i++) {
        if (g_order_len < (int)sizeof(g_order) - 1)
            g_order[g_order_len++] = 'B';
        osYieldThread();
    }
}

static void body_c(void *arg)
{
    int i;
    (void)arg;
    for (i = 0; i < 3; i++) {
        if (g_order_len < (int)sizeof(g_order) - 1)
            g_order[g_order_len++] = 'C';
        osYieldThread();
    }
}

static void starter_body(void *arg)
{
    (void)arg;
    osCreateThread(&th_a, 1, body_a, NULL, NULL, 10);
    osCreateThread(&th_b, 2, body_b, NULL, NULL, 20);
    osCreateThread(&th_c, 3, body_c, NULL, NULL, 20);
    osStartThread(&th_a);
    osStartThread(&th_b);
    osStartThread(&th_c);
    osStopThread(NULL);
}

/* ===================== T1 bodies ===================== */
static void body_t1a(void *arg)
{
    OSMesg m;
    int i;
    (void)arg;
    for (i = 0; i < 1000; i++) {
        osRecvMesg(&t1_qba, &m, OS_MESG_BLOCK);
        osSendMesg(&t1_qab, m, OS_MESG_BLOCK);
        t1_count++;
    }
    t1_done = 1;
}

static void body_t1b(void *arg)
{
    OSMesg m;
    int i;
    (void)arg;
    for (i = 0; i < 1000; i++) {
        osRecvMesg(&t1_qab, &m, OS_MESG_BLOCK);
        osSendMesg(&t1_qba, m, OS_MESG_BLOCK);
    }
}

/* ===================== T3 bodies ===================== */
static void body_t3h(void *arg)
{
    OSMesg m;
    (void)arg;
    osRecvMesg(&t3_q, &m, OS_MESG_BLOCK);
    t3_counter++;
}

static void body_t3l(void *arg)
{
    (void)arg;
    osSendMesg(&t3_q, (OSMesg)1, OS_MESG_BLOCK);
    /* H (higher priority) must have run and incremented before we get here */
}

static void starter_t3_body(void *arg)
{
    (void)arg;
    osCreateThread(&th_t3h, 10, body_t3h, NULL, NULL, 30);
    osCreateThread(&th_t3l, 11, body_t3l, NULL, NULL, 10);
    osStartThread(&th_t3h);
    osStartThread(&th_t3l);
    osStopThread(NULL);
}

/* ===================== T5/T6/T7 bodies ===================== */
static void body_t5(void *arg)
{
    OSMesg m;
    OSTime t0;
    (void)arg;
    t0 = osGetTime();
    while (osGetTime() - t0 < (OSTime)OS_CPU_COUNTER) {
        osRecvMesg(&t5_q, &m, OS_MESG_BLOCK);
        t5_count++;
    }
    t5_done = 1;
}

static void body_t6one(void *arg)
{
    OSMesg m;
    OSTime start;
    (void)arg;
    start = osGetTime();
    osRecvMesg(&t6_one_q, &m, OS_MESG_BLOCK);
    t6_one_elapsed = osGetTime() - start;
    t6_one_done = 1;
}

static void body_t6per(void *arg)
{
    OSMesg m;
    OSTime t0;
    (void)arg;
    t0 = osGetTime();
    while (osGetTime() - t0 < (OSTime)OS_CPU_COUNTER) {
        osRecvMesg(&t6_per_q, &m, OS_MESG_BLOCK);
        t6_per_count++;
    }
    t6_per_done = 1;
}

static void body_t7(void *arg)
{
    unsigned long long w0, w1;
    OSTime o0, o1;
    (void)arg;
    w0 = port_wall_ticks_now();
    o0 = osGetTime();
    while (port_wall_ticks_now() - w0 < WALL_CLOCK) { }
    w1 = port_wall_ticks_now();
    o1 = osGetTime();
    t7_delta = o1 - o0;
    t7_done = 1;
}

/* ---------- entry point (called from main() on the DOL main thread) ---------- */
static void selftest_body(void *arg)
{
    (void)arg;
    int pass = 1;

    /* ---------- T1: ping-pong 1000 messages between two pri-20 threads ----------
     * Seed qBA with one token so A can start; A recv qBA / send qAB, B recv qAB /
     * send qBA. Both size-1 queues, both threads pri 20 (equal). A counts its
     * receives; when it reaches 1000 it sets t1_done. main polls with a time cap. */
    {
        OSMesg seed = 0;
        t1_count = 0;
        t1_done = 0;
        osCreateMesgQueue(&t1_qab, t1_qab_buf, 1);
        osCreateMesgQueue(&t1_qba, t1_qba_buf, 1);
        osSendMesg(&t1_qba, seed, OS_MESG_NOBLOCK); /* seed */
        osCreateThread(&th_t1a, 4, body_t1a, NULL, NULL, 20);
        osCreateThread(&th_t1b, 5, body_t1b, NULL, NULL, 20);
        osStartThread(&th_t1a);
        osStartThread(&th_t1b);
        {
            OSTime t0 = osGetTime();
            while (!t1_done) { 
                if (osGetTime() - t0 > (OSTime)OS_CPU_COUNTER * 10) break;
            }
        }
        if (t1_done && t1_count == 1000) {
            PORT_LOG("T1 ok");
        } else {
            PORT_LOG("T1 FAIL %d", t1_count);
            pass = 0;
        }
    }

    /* ---------- T2: NOBLOCK recv on empty -> -1; NOBLOCK send on full -> -1 ----------
     * Runs on the main thread (not a game thread); NOBLOCK never parks the caller. */
    {
        OSMesgQueue q2;
        OSMesg      buf2[1];
        OSMesg      got;
        s32 r_recv, r_send;
        osCreateMesgQueue(&q2, buf2, 1);
        r_recv = osRecvMesg(&q2, &got, OS_MESG_NOBLOCK);   /* empty -> -1 */
        osSendMesg(&q2, (OSMesg)7, OS_MESG_NOBLOCK);        /* fill it */
        r_send = osSendMesg(&q2, (OSMesg)8, OS_MESG_NOBLOCK); /* full -> -1 */
        if (r_recv == -1 && r_send == -1) {
            PORT_LOG("T2 ok");
        } else {
            PORT_LOG("T2 FAIL %d %d", r_recv, r_send);
            pass = 0;
        }
    }

    /* ---------- T3: preemption at the send boundary ----------
     * H(30) blocks on recv of an empty queue; L(10) sends to it. The send wakes
     * H, which outranks L, so H runs (counter++) before osSendMesg returns. L
     * then checks the counter is already 1. */
    {
        t3_counter = 0;
        osCreateMesgQueue(&t3_q, t3_q_buf, 1);
        osCreateThread(&th_t3s, 12, starter_t3_body, NULL, NULL, 50);
        osStartThread(&th_t3s);
        {
            OSTime t0 = osGetTime();
            while (t3_counter < 1) {
                if (osGetTime() - t0 > (OSTime)OS_CPU_COUNTER * 5) break;
            }
        }
        if (t3_counter == 1) {
            PORT_LOG("T3 ok");
        } else {
            PORT_LOG("T3 FAIL %d", t3_counter);
            pass = 0;
        }
    }

    /* ---------- T4: priority order log ---------- */
    {
        const char *expect = "BCBCBCAAA";
        int ok;
        g_order_len = 0;
        g_order[0] = '\0';
        osCreateThread(&th_s, 0, starter_body, NULL, NULL, 50);
        osStartThread(&th_s);
        {
            OSTime t0 = osGetTime();
            while (g_order_len < 9) {
                if (osGetTime() - t0 > (OSTime)OS_CPU_COUNTER * 5) break;
            }
        }
        g_order[g_order_len] = '\0';
        ok = (g_order_len == 9) && (g_order[0] != '\0');
        {
            int i;
            for (i = 0; i < 9 && ok; i++)
                if (g_order[i] != expect[i]) ok = 0;
        }
        if (ok) {
            PORT_LOG("T4 ok %s", g_order);
        } else {
            PORT_LOG("T4 FAIL %s", g_order);
            pass = 0;
        }
    }

    /* ---------- T5: VI events drive a pri-20 thread's BLOCK recv ----------
     * osViSetEvent posts one message per retrace (retraceCount=1). The thread
     * counts messages for 1s of osGetTime; at ~60Hz that is >= 50. */
    {
        t5_count = 0;
        t5_done = 0;
        osCreateMesgQueue(&t5_q, t5_q_buf, 8);
        osViSetEvent(&t5_q, (OSMesg)1, 1);
        osCreateThread(&th_t5, 20, body_t5, NULL, NULL, 20);
        osStartThread(&th_t5);
        {
            OSTime t0 = osGetTime();
            while (!t5_done) { 
                if (osGetTime() - t0 > (OSTime)OS_CPU_COUNTER * 5) break;
            }
        }
        if (t5_done && t5_count >= 50) {
            PORT_LOG("T5 ok %d", t5_count);
        } else {
            { extern void port_events_debug(unsigned*,unsigned*,unsigned*,unsigned*,unsigned*); unsigned a,b,c,d,e; port_events_debug(&a,&b,&c,&d,&e); PORT_LOG("T5 FAIL %d (vi_cb=%u posts=%u pump_wakes=%u ring=%u/%u)", t5_count,a,b,c,d,e); }
            pass = 0;
        }
    }

    /* ---------- T6a: one-shot timer fires ~100ms after osSetTimer ---------- */
    {
        OSTime lo = (OSTime)OS_CPU_COUNTER / 10 * 9 / 10;  /* 90 ms */
        OSTime hi = (OSTime)OS_CPU_COUNTER / 10 * 15 / 10; /* 150 ms */
        t6_one_elapsed = 0;
        t6_one_done = 0;
        osCreateMesgQueue(&t6_one_q, t6_one_buf, 1);
        osSetTimer(&t6_one_timer, (OSTime)OS_CPU_COUNTER / 10, 0,
                   &t6_one_q, (OSMesg)1);
        osCreateThread(&th_t6one, 21, body_t6one, NULL, NULL, 20);
        osStartThread(&th_t6one);
        {
            OSTime t0 = osGetTime();
            while (!t6_one_done) { 
                if (osGetTime() - t0 > (OSTime)OS_CPU_COUNTER * 5) break;
            }
        }
        if (t6_one_done && t6_one_elapsed >= lo && t6_one_elapsed <= hi) {
            PORT_LOG("T6 ok %lld", (long long)t6_one_elapsed);
        } else {
            PORT_LOG("T6 FAIL %lld", (long long)t6_one_elapsed);
            pass = 0;
        }
    }

    /* ---------- T6b: periodic 50ms timer -> 18..22 fires in 1s ---------- */
    {
        t6_per_count = 0;
        t6_per_done = 0;
        osCreateMesgQueue(&t6_per_q, t6_per_buf, 1);
        osSetTimer(&t6_per_timer, (OSTime)OS_CPU_COUNTER / 20,
                   (OSTime)OS_CPU_COUNTER / 20, &t6_per_q, (OSMesg)1);
        osCreateThread(&th_t6per, 22, body_t6per, NULL, NULL, 20);
        osStartThread(&th_t6per);
        {
            OSTime t0 = osGetTime();
            while (!t6_per_done) { 
                if (osGetTime() - t0 > (OSTime)OS_CPU_COUNTER * 5) break;
            }
        }
        if (t6_per_done && t6_per_count >= 18 && t6_per_count <= 22) {
            PORT_LOG("T6 ok %d", t6_per_count);
        } else {
            PORT_LOG("T6 FAIL %d", t6_per_count);
            pass = 0;
        }
    }

    /* ---------- T7: osGetTime advances 46.875e6 +/-5% per wall second ------ */
    {
        OSTime lo = (OSTime)OS_CPU_COUNTER - (OSTime)OS_CPU_COUNTER / 20;
        OSTime hi = (OSTime)OS_CPU_COUNTER + (OSTime)OS_CPU_COUNTER / 20;
        t7_delta = 0;
        t7_done = 0;
        osCreateThread(&th_t7, 23, body_t7, NULL, NULL, 20);
        osStartThread(&th_t7);
        {
            OSTime t0 = osGetTime();
            while (!t7_done) { 
                if (osGetTime() - t0 > (OSTime)OS_CPU_COUNTER * 5) break;
            }
        }
        if (t7_done && t7_delta >= lo && t7_delta <= hi) {
            PORT_LOG("T7 ok %lld", (long long)t7_delta);
        } else {
            PORT_LOG("T7 FAIL %lld", (long long)t7_delta);
            pass = 0;
        }
    }

    if (pass)
        PORT_LOG("SELFTEST PASS");
    else
        PORT_LOG("SELFTEST FAIL");
    { extern void port_log_flush(void); port_log_flush(); }
}

/* The test driver must itself be a game thread (main() is not one): it runs at
 * a low N64 priority and its busy-waits call osGetTime, a preemption point. */
static OSThread th_driver;
void port_selftest(void)
{
    osCreateThread(&th_driver, 99, selftest_body, NULL, NULL, 5);
    osStartThread(&th_driver);
}
