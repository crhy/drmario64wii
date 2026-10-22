/* Dr. Mario 64 -> Wii native port: SD card logging (task 02b, hardened task 07,
 * cheap logging task 12d).
 *  Mounts the Wii SD card with libfat, creates sd:/drmario/ and appends
 *  sd:/drmario/log.txt. Safe no-op if the SD card is missing.
 *
 *  Thread-safety (task 07): one LWP mutex guards the whole open/write/close
 *  sequence so only one thread touches the file at a time. The mutex is now
 *  initialised inside port_log_init (not lazily): lazy init raced when two
 *  threads logged for the first time at once.
 *
 *  Cheap logging (task 12d): the FILE is fully buffered with a 16 KB static
 *  buffer, and fflush+fsync (an SD sector write) only fires when 32 lines are
 *  pending, when 1 second (GetClock microseconds) has passed since the last
 *  sync, or when the line starts with "CRASH". port_log_flush() forces a sync
 *  and is called from port_crash_report before parking.
 *
 *  Auto-flush (task 12f3): the buffered tail used to die with the program, so
 *  the LAST lines before a hang/exit (the self-test's "T7 ok"/"SELFTEST PASS")
 *  never reached the SD card. A priority-10 LWP started by port_log_init
 *  sleeps 1 s (usleep) in a loop and calls port_log_flush() whenever lines
 *  are pending (logPending > 0), so the tail lands on the card on its own.
 */

#include <fat.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <ogc/lwp.h>
#include <ogc/mutex.h>
#include <unistd.h>
#include "heap_check.h"
extern void port_guard_canary(const char *name, void *stack_low);
extern void port_guard_range(const char *name, const void *low, unsigned size);

static bool logReady = false;
static mutex_t logMutex;
static FILE *logFile = NULL;
static char logBuf[16384];
static unsigned logPending = 0;
static unsigned long long logLastSync = 0;

/* Auto-flush LWP (task 12f3): static 16 KB stack, priority 10. */
static unsigned char logFlushStack[16384 + 32] __attribute__((aligned(32)));
static lwp_t logFlushLwp;

/* Time base: PowerPC TBL ticks at PPC_TIMER_CLOCK = 60.75e6/s on Wii
 * (same source as os_events.c's gettime()). */
static inline unsigned long long log_gettime(void)
{
    unsigned int hi, lo, hi2;
    do {
        __asm__ __volatile__("mftbu %0" : "=r"(hi));
        __asm__ __volatile__("mftb %0" : "=r"(lo));
        __asm__ __volatile__("mftbu %0" : "=r"(hi2));
    } while (hi != hi2);
    return ((unsigned long long)hi << 32) | lo;
}

static void *logFlusherBody(void *arg);
void PORT_LOG(const char *fmt, ...);

int port_log_init(void)
{
    if (!fatInitDefault())
        return 0;
    mkdir("sd:/drmario", 0777);
    logFile = fopen("sd:/drmario/log.txt", "a");
    if (logFile == NULL)
        return 0;
    setvbuf(logFile, logBuf, _IOFBF, sizeof logBuf);
    LWP_MutexInit(&logMutex, false);
    logLastSync = log_gettime();
    logReady = true;
    LWP_CreateThread(&logFlushLwp, logFlusherBody, NULL,
                     (void *)(((unsigned long)&logFlushStack[0] + 31u) & ~(unsigned long)31u),
                     16384, 90);
    port_guard_canary("log", logFlushStack);
    port_guard_range("log", logFlushStack, sizeof(logFlushStack));
    return 1;
}

void port_log_flush(void);

/* Sleep 1 s, then force a sync (unconditional: the one-flush lag in libfat
 * stdio means the last fflush+fsync loses its content, so we need a second
 * flush to commit it; logPending may already be 0 when this fires). */
static void *logFlusherBody(void *arg)
{
    (void)arg;
#if defined(PORT_SCHED_DUMP) || defined(PORT_SCHED_DUMP2)
    unsigned n = 0;
#endif
    for (;;) {
        usleep(1000000u);
        port_heap_guard_check();
#if defined(PORT_SCHED_DUMP) || defined(PORT_SCHED_DUMP2)
        { extern void port_sched_dump(unsigned); if ((++n % 10u) == 0u) port_sched_dump(n); }
#endif
        if (logReady && logFile != NULL) {
            LWP_MutexLock(logMutex);
            fflush(logFile);
            fsync(fileno(logFile));
            logLastSync = log_gettime();
            LWP_MutexUnlock(logMutex);
        }
    }
    return NULL;
}

static bool logNeedsSync(const char *fmt, unsigned long long *now)
{
    *now = log_gettime();
    if (fmt[0] == 'C' && fmt[1] == 'R' && fmt[2] == 'A' && fmt[3] == 'S' && fmt[4] == 'H')
        return true;
    if (logPending >= 32u)
        return true;
    if ((*now - logLastSync) >= 60750000ULL)
        return true;
    return false;
}

void PORT_LOG(const char *fmt, ...)
{
    va_list args;
    unsigned long long now;

    if (!logReady)
        return;

    LWP_MutexLock(logMutex);
    {
        if (logFile != NULL) {
            bool sync = logNeedsSync(fmt, &now);
            va_start(args, fmt);
            vfprintf(logFile, fmt, args);
            va_end(args);
            fputc('\n', logFile);
            logPending++;
            if (sync) {
                fflush(logFile);
                fsync(fileno(logFile));
                logPending = 0;
                logLastSync = now;
            }
        }
    }
    LWP_MutexUnlock(logMutex);
}

void port_log_flush(void)
{
    if (!logReady)
        return;

    LWP_MutexLock(logMutex);
    {
        if (logFile != NULL) {
            fflush(logFile);
            fsync(fileno(logFile));
            /* libfat/newlib stdio one-flush lag: the first fflush+fsync
             * commits the PREVIOUS write; a second sync commits THIS one. */
            fflush(logFile);
            fsync(fileno(logFile));
            logPending = 0;
            logLastSync = log_gettime();
        }
    }
    LWP_MutexUnlock(logMutex);
}
