# Task 05: libultra threads, messages, events and time on libogc
Create `port/ultra/os_thread.c` and `port/ultra/os_mesg.c` (add to WII_SRCS). Game code sees the libultra types
from include/ (OSThread, OSMesgQueue, OSMesg, OSTime, OSPri, OSId); do NOT change those structs: keep port state
in a side table keyed by the OSThread* / OSMesgQueue* pointer (fixed arrays of 32 entries are enough) or inside
fields the struct already has. port files may include both libogc (<ogc/lwp.h>, <ogc/mutex.h>, <ogc/cond.h>,
<ogc/lwp_watchdog.h>) and the game's ultra headers; if names clash, split into a small "backend" file that only
includes libogc.
Implement with libultra semantics:
- osCreateThread(t,id,entry,arg,sp,pri): create an LWP thread with its own 64 KB stack (ignore sp), starting
  SUSPENDED: the LWP waits on a per-thread semaphore until osStartThread(t). osStartThread/osStopThread
  (stop = LWP_SuspendThread unless it is the current thread, then suspend self), osSetThreadPri, osGetThreadPri,
  osGetThreadId, osYieldThread, osDestroyThread (no-op is fine). Priority map: lwp_pri = 1 + (pri*126)/255, keep order.
- osCreateMesgQueue, osSendMesg, osJamMesg, osRecvMesg with OS_MESG_BLOCK / OS_MESG_NOBLOCK and return values
  exactly as libultra (0 ok, -1 full/empty when NOBLOCK). Use one global mutex + one condvar per queue
  (validCount, first, msgCount, msg[] fields of OSMesgQueue are kept up to date as libultra does).
- osSetEventMesg(event,mq,msg): store; `port_os_event(int event)` sends the stored message (NOBLOCK) — later tasks
  call it for OS_EVENT_SP / OS_EVENT_DP / OS_EVENT_SI / OS_EVENT_PI.
- osViSetEvent(mq,msg,retraceCount): register; a VIDEO_SetPostRetraceCallback callback sends msg every
  retraceCount retraces (from IRQ context use a NOBLOCK path that never blocks or takes a sleeping lock: use
  a counter + LWP semaphore/thread that does the send).
- osGetTime / osGetCount: N64 count runs at 46.875 MHz (OS_CPU_COUNTER); convert from gettime()/ticks_to_nanosecs.
- osSetTimer/osStopTimer: one LWP timer thread that sleeps until the earliest expiry and sends the messages.
- osGetThreadPri(NULL)/osSetThreadPri(NULL,..) mean the current thread.
Write a self-test `port/ultra/test_threads.c` compiled only when `WII_SELFTEST=1` (Makefile.wii flag) that runs
from port/wii/main.c: two threads ping-pong 1000 messages, a NOBLOCK recv on empty returns -1, a VI event
counts >= 50 retraces in 1 s, osGetTime advances ~46.875e6 per second (+-5%). It logs "SELFTEST PASS" or
"SELFTEST FAIL <reason>" with PORT_LOG (port/include/port_log.h; if it does not exist yet, create it with a PORT_LOG
that writes to sd:/drmario/log.txt through libfat, fflush per line).
GATE: make -f Makefile.wii clean; make -f Makefile.wii WII_SELFTEST=1 && .strata/dolphin-run.sh build/wii/drmario64.dol 25 .strata/scratch/t05 && grep -q "SELFTEST PASS" .strata/scratch/t05/sd/drmario/log.txt && ! grep -q "SELFTEST FAIL" .strata/scratch/t05/sd/drmario/log.txt
