#include "boot_main.h"

#include "libc/stdbool.h"

#include "libultra.h"
#include "dma_table.h"
#include "segment_symbols.h"
#include "macros_defines.h"
#include "dmadata.h"
#include "dm_thread.h"
#include "util.h"
#ifdef TARGET_WII
#include "port_log.h"
#endif

void Idle_ThreadEntry(void *arg);

static OSThread sIdleThread;
static OSThread sMainThread;
#ifdef TARGET_WII
static STACK(sIdleStack, 0x10000);
static STACK(sMainStack, 0x10000);
#else
static STACK(sIdleStack, 0x2000);
static STACK(sMainStack, 0x2000);
#endif
STACK(gBootThreadStack, BOOT_STACK_SIZE);

void Idle_Nop(void) {
}

void (*D_8000E190)(void) = Idle_Nop;

void Main_StopThread(void) {
    osStopThread(&sMainThread);
}

void func_80000488(void (*arg0)(void)) {
    D_8000E190 = arg0;
}

void bootproc(void) {
    osInitialize();
    osCreateThread(&sIdleThread, THREAD_ID_IDLE, Idle_ThreadEntry, NULL, STACK_TOP(sIdleStack), THREAD_PRI_IDLE_BOOT);
    osStartThread(&sIdleThread);
}

void Main_ThreadEntry(void *arg) {
#ifdef TARGET_WII
    /* The DOL loader already placed the main segment and zeroed .bss, and
     * gMainSegmentDmaInfo.entry is statically initialized to mainproc, so
     * there is no ROM to DMA/decompress. Call the entry function directly. */
    PORT_LOG("boot:entry");
    gMainSegmentDmaInfo.entry(arg);
#else
    RomOffset mainSegmentStart;

    DmaData_RomToRam(SEGMENT_ROM_START(dma_table), &gMainSegmentDmaInfo, SEGMENT_ROM_SIZE(dma_table));

    mainSegmentStart = SEGMENT_ROM_START(main_segment);

#if VERSION_CN
    do {
#endif
        DecompressRomToRam(mainSegmentStart, gMainSegmentDmaInfo.vram, SEGMENT_ROM_SIZE(main_segment));

        bzero(gMainSegmentDmaInfo.bssStart, gMainSegmentDmaInfo.bssEnd - gMainSegmentDmaInfo.bssStart);
#if VERSION_CN
    } while (0);
#endif

    gMainSegmentDmaInfo.entry(arg);
#endif
}

OSMesg sPiMgrCmdBuff[SPIMGRCMDBUFF_LEN];
OSMesgQueue sPiMgrCmdQueue;

void Idle_ThreadEntry(void *arg) {
#ifdef TARGET_WII
    PORT_LOG("idle:entry");
#endif
    osCreatePiManager(OS_PRIORITY_PIMGR, &sPiMgrCmdQueue, sPiMgrCmdBuff, ARRAY_COUNT(sPiMgrCmdBuff));
    osCreateThread(&sMainThread, THREAD_ID_MAIN, Main_ThreadEntry, arg, STACK_TOP(sMainStack), THREAD_PRI_MAIN);
    osStartThread(&sMainThread);
#ifdef TARGET_WII
    PORT_LOG("idle:main-started");
    /* On Wii the idle thread has no work. osStopThread(NULL) suspends the
     * calling (idle) thread forever without spinning, so it cannot starve
     * the game threads. The VI is driven by the game's scheduler via
     * port_present(), not by a pump. */
    osStopThread(NULL);
#else
    while (true) {
        D_8000E190();
    }
#endif
}
