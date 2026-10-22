# Task 06b2: N64-style scheduler, part 2 of 3: message queues
Design: `.strata/scratch/06b-spec-full.md`. Task 06b1 wrote port/ultra/os_sched.c (threads). Now move ALL message-queue
functions into os_sched.c and delete the old ones (port/ultra/os_mesg.c and the queue parts of os_backend.c):
osCreateMesgQueue, osSendMesg, osJamMesg (inserts at the front), osRecvMesg, MQ_IS_EMPTY/FULL semantics as libultra
(fields validCount, first, msgCount, msg). Waiters: side table queue -> waiting SchedThreads (recv waiters and send
waiters). BLOCK recv on empty: sched_block_current_locked() and retry when woken. A send that wakes a waiter makes it
READY and calls the preemption check (a higher-priority waiter runs before osSendMesg returns). NOBLOCK returns -1
on empty/full. All under sched_lock.
Self-test: add tests T1 (ping-pong 1000 messages between two pri-20 threads), T2 (NOBLOCK recv on empty and NOBLOCK
send on full both return -1), T3 (low-pri L sends to a queue that high-pri H waits on; H increments a counter; L checks
the counter is already 1 right after its osSendMesg returns). Keep T4. Log "T1 ok"... "SELFTEST PASS".
GATE: make -f Makefile.wii clean >/dev/null; make -f Makefile.wii WII_SELFTEST=1 >/dev/null && .strata/dolphin-run.sh build/wii/drmario64.dol 20 .strata/scratch/t06b2 && grep -a "T[0-9]" .strata/scratch/t06b2/sd/drmario/log.txt; for t in T1 T2 T3 T4; do grep -aq "$t ok" .strata/scratch/t06b2/sd/drmario/log.txt || exit 1; done
