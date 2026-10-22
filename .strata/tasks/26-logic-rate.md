# Task 26: game logic runs at 15 fps instead of 60 (main loop gets a retrace message only every 4th VI)
Measured (Claude): in dm_game_main's loop (src/main_segment/dm_game_main.c) osRecvMesg(&scMQ) returns a retrace
message only every 4 VI frames, the queue is empty each time (so messages are not piling up, they are not SENT).
The game therefore runs at 1/4 speed. Not the cause: retraceMQ size (tested 8), gfx task time (~4.6 ms), present (~10 us).
Find who stops the per-VI broadcast and fix it. Path: VI callback (port/ultra/os_events.c vi_retrace_callback ->
port_post_event) -> pump -> retraceMQ -> nnScEventHandler (src/main_segment/nnsched.c, thread 19) ->
nnScEventBroadcast -> client queues. Log (#ifdef PORT_LOG_VERBOSE-free, once-per-second counters) how many VIDEO_MSG
are posted, delivered to retraceMQ, dropped (MQ full), received by thread 19, and broadcast per second. Likely suspects:
messages dropped because a game thread runs several VIs without hitting a scheduling point, or retraceCount/g_vi_retrace.
GATE: make -f Makefile.wii clean >/dev/null && make 2>&1 | grep -q ": OK" && make -f Makefile.wii EXTRA_CFLAGS=-DPORT_INPUT_SCRIPT PORT_INPUT_SCRIPT=.strata/scratch/sync/1p.txt >/dev/null 2>&1 && grep -q 'logic n=' src/main_segment/dm_game_main.c && .strata/dolphin-run.sh build/wii/drmario64.dol 60 .strata/scratch/t26 >/dev/null; python3 -c "import re; t=open('.strata/scratch/t26/sd/drmario/log.txt','rb').read().replace(b'\0',b'').decode('latin-1'); v=[(int(a),int(b)) for a,b in re.findall(r'logic n=(\d+) vi=(\d+)',t)]; r=(v[-1][0]-v[0][0])/(v[-1][1]-v[0][1]); print('logic per vi',r); assert r>0.9"
Also add (#ifdef TARGET_WII) in that loop after dm_game_main2(): every 60 calls PORT_LOG("logic n=%u vi=%u", n, port_frame_now()).
