# NOTES - recon for the Wii port (task 01)

## libultra API used
  alAdpcmPull:  lib/libmus/src/synthInternals.h
  alAudioFrame:  lib/libmus/include/PR/n_libaudio_sn_sc.h lib/libmus/src/aud_thread.c lib/libmus/src/n_libaudio_sn_sc.h
  alAuxBusNew:  lib/libmus/src/player_fx.c lib/libmus/src/synthInternals.h
  alAuxBusParam:  lib/libmus/src/player_fx.c lib/libmus/src/synthInternals.h
  alAuxBusPull:  lib/libmus/src/synthInternals.h
  alClose:  lib/libmus/include/PR/n_libaudio_sn_sc.h lib/libmus/src/n_libaudio_sn_sc.h
  alEnvmixerNew:  lib/libmus/src/player_fx.c lib/libmus/src/synthInternals.h
  alEnvmixerParam:  lib/libmus/src/player_fx.c lib/libmus/src/synthInternals.h
  alEnvmixerPull:  lib/libmus/src/synthInternals.h src/main_segment/audio/sound.c
  alFilterNew:  lib/libmus/src/player_fx.c lib/libmus/src/synthInternals.h
  alFxNew:  lib/libmus/src/synthInternals.h
  alFxParamHdl:  lib/libmus/src/player_fx.c lib/libmus/src/synthInternals.h
  alFxParam:  lib/libmus/src/player_fx.c lib/libmus/src/synthInternals.h
  alFxPull:  lib/libmus/src/player_fx.c lib/libmus/src/synthInternals.h
  alGlobals:  lib/libmus/src/player_fx.c
  alHeapAlloc:  lib/libmus/src/lib_memory.c lib/libmus/src/player_fx.c
  alHeapInit:  lib/libmus/src/lib_memory.c
  alInit:  lib/libmus/include/PR/n_libaudio_sn_sc.h lib/libmus/src/aud_thread.c lib/libmus/src/n_libaudio_sn_sc.h lib/libmus/src/player_fx.c lib/libmus/src/player_fx.h
  alLink:  lib/libmus/src/player_fx.c
  alLoadNew:  lib/libmus/src/player_fx.c lib/libmus/src/synthInternals.h
  alLoadParam:  lib/libmus/src/player_fx.c lib/libmus/src/synthInternals.h
  alMainBusNew:  lib/libmus/src/player_fx.c lib/libmus/src/synthInternals.h
  alMainBusParam:  lib/libmus/src/player_fx.c lib/libmus/src/synthInternals.h
  alMainBusPull:  lib/libmus/src/synthInternals.h
  alN_PVoiceNew:  lib/libmus/src/n_synthInternals.h lib/libmus/src/player_fx.c
  alRaw16Pull:  lib/libmus/src/synthInternals.h
  alResampleNew:  lib/libmus/src/player_fx.c lib/libmus/src/synthInternals.h
  alResampleParam:  lib/libmus/src/player_fx.c lib/libmus/src/synthInternals.h
  alResamplePull:  lib/libmus/src/synthInternals.h
  alSaveNew:  lib/libmus/src/player_fx.c lib/libmus/src/synthInternals.h
  alSaveParam:  lib/libmus/src/player_fx.c lib/libmus/src/synthInternals.h
  alSavePull:  lib/libmus/src/synthInternals.h
  alSynAddPlayer:  lib/libmus/include/PR/n_libaudio_sn_sc.h lib/libmus/src/n_libaudio_sn_sc.h lib/libmus/src/player_api.c
  alSynAllocFX:  lib/libmus/include/PR/n_libaudio_sn_sc.h lib/libmus/src/n_libaudio_sn_sc.h lib/libmus/src/player_fx.c lib/libmus/src/player_fx.h
  alSynAllocVoice:  lib/libmus/include/PR/n_libaudio_sn_sc.h lib/libmus/src/n_libaudio_sn_sc.h lib/libmus/src/player_api.c
  alSynDelete:  lib/libmus/include/PR/n_libaudio_sn_sc.h lib/libmus/src/n_libaudio_sn_sc.h
  alSynFreeVoice:  lib/libmus/include/PR/n_libaudio_sn_sc.h lib/libmus/src/n_libaudio_sn_sc.h
  alSynGetFXRef:  lib/libmus/include/PR/n_libaudio_sn_sc.h lib/libmus/src/n_libaudio_sn_sc.h
  alSynGetPriority:  lib/libmus/include/PR/n_libaudio_sn_sc.h lib/libmus/src/n_libaudio_sn_sc.h
  alSynNew:  lib/libmus/include/PR/n_libaudio_sn_sc.h lib/libmus/src/n_libaudio_sn_sc.h lib/libmus/src/player_fx.c lib/libmus/src/player_fx.h
  alSynRemovePlayer:  lib/libmus/include/PR/n_libaudio_sn_sc.h lib/libmus/src/n_libaudio_sn_sc.h lib/libmus/src/player.c
  alSynSetFXMix:  lib/libmus/include/PR/n_libaudio_sn_sc.h lib/libmus/src/n_libaudio_sn_sc.h lib/libmus/src/player.c
  alSynSetFXParam:  lib/libmus/include/PR/n_libaudio_sn_sc.h lib/libmus/src/n_libaudio_sn_sc.h
  alSynSetFXtype:  lib/libmus/include/PR/n_libaudio_sn_sc.h lib/libmus/src/n_libaudio_sn_sc.h
  alSynSetMix:  lib/libmus/src/player.h
  alSynSetPan:  lib/libmus/include/PR/n_libaudio_sn_sc.h lib/libmus/src/n_libaudio_sn_sc.h lib/libmus/src/player.c
  alSynSetPitch:  lib/libmus/include/PR/n_libaudio_sn_sc.h lib/libmus/src/n_libaudio_sn_sc.h lib/libmus/src/player.c
  alSynSetPriority:  lib/libmus/include/PR/n_libaudio_sn_sc.h lib/libmus/src/n_libaudio_sn_sc.h
  alSynSetVol:  lib/libmus/include/PR/n_libaudio_sn_sc.h lib/libmus/src/n_libaudio_sn_sc.h lib/libmus/src/player.c
  alSynStartVoice:  lib/libmus/include/PR/n_libaudio_sn_sc.h lib/libmus/src/n_libaudio_sn_sc.h lib/libmus/src/player.c
  alSynStartVoiceParams:  lib/libmus/include/PR/n_libaudio_sn_sc.h lib/libmus/src/n_libaudio_sn_sc.h
  alSynStopVoice:  lib/libmus/include/PR/n_libaudio_sn_sc.h lib/libmus/src/n_libaudio_sn_sc.h lib/libmus/src/player.c
  gspF3DEX2_fifoDataStart:  src/main_segment/graphic.c
  gspF3DEX2_fifoTextStart:  src/main_segment/graphic.c
  gspS2DEX_fifoDataStart:  src/main_segment/graphic.c
  gspS2DEX_fifoTextStart:  src/main_segment/graphic.c
  guMtxCatF:  src/main_segment/unused/020D10.c
  guMtxF2L:  src/main_segment/dm_game_main.c src/main_segment/dm_manual_main.c src/main_segment/main_menu.c src/main_segment/unused/020D10.c src/main_segment/unused/065080.c
  guMtxIdent:  src/main_segment/unused/020D10.c
  guMtxL2F:  src/main_segment/unused/065080.c
  guOrtho:  src/main_segment/dm_game_main.c src/main_segment/dm_manual_main.c src/main_segment/game_etc.c src/main_segment/main_menu.c src/main_segment/main_story.c
  guPerspectiveF:  src/main_segment/unused/020D10.c
  guPerspective:  src/main_segment/main_story.c
  guRotateRPYF:  src/main_segment/dm_game_main.c src/main_segment/dm_manual_main.c src/main_segment/main_menu.c src/main_segment/unused/020D10.c
  guRotateRPY:  src/main_segment/unused/020D10.c
  guScaleF:  src/main_segment/unused/020D10.c
  guScale:  src/main_segment/unused/020D10.c
  guTranslateF:  src/main_segment/unused/020D10.c
  guTranslate:  src/main_segment/dm_game_main.c src/main_segment/dm_manual_main.c src/main_segment/main_menu.c
  n_alAdpcmPull:  lib/libmus/src/n_synthInternals.h
  n_alAudioFrame:  lib/libmus/include/PR/n_libaudio_sc.h lib/libmus/include/PR/n_libaudio_sn_sc.h lib/libmus/src/n_libaudio_sc.h lib/libmus/src/n_libaudio_sn_sc.h
  n_alAuxBusPull:  lib/libmus/src/n_synthInternals.h lib/libmus/src/player_fx.c
  n_alClose:  lib/libmus/include/PR/n_libaudio_sc.h lib/libmus/include/PR/n_libaudio_sn_sc.h lib/libmus/src/n_libaudio_sc.h lib/libmus/src/n_libaudio_sn_sc.h
  n_alEnvmixerParam:  lib/libmus/src/n_synthInternals.h
  n_alEnvmixerPull:  lib/libmus/src/n_synthInternals.h
  n_alFxNew:  lib/libmus/src/n_synthInternals.h
  n_alFxParamHdl:  lib/libmus/src/n_synthInternals.h
  n_alFxPull:  lib/libmus/src/n_synthInternals.h lib/libmus/src/player_fx.c
  n_alGlobals:  lib/libmus/include/PR/n_libaudio_sc.h lib/libmus/src/n_libaudio_sc.h lib/libmus/src/player_fx.c
  n_alInit:  lib/libmus/include/PR/n_libaudio_sc.h lib/libmus/include/PR/n_libaudio_sn_sc.h lib/libmus/src/n_libaudio_sc.h lib/libmus/src/n_libaudio_sn_sc.h lib/libmus/src/player_fx.h
  n_alLoadParam:  lib/libmus/src/n_synthInternals.h
  n_alMainBusPull:  lib/libmus/src/n_synthInternals.h
  n_alResampleParam:  lib/libmus/src/n_synthInternals.h
  n_alResamplePull:  lib/libmus/src/n_synthInternals.h
  n_alSavePull:  lib/libmus/src/n_synthInternals.h
  n_alSynAddPlayer:  lib/libmus/include/PR/n_libaudio_sc.h lib/libmus/include/PR/n_libaudio_sn_sc.h lib/libmus/src/n_libaudio_sc.h lib/libmus/src/n_libaudio_sn_sc.h
  n_alSynAllocFX:  lib/libmus/include/PR/n_libaudio_sc.h lib/libmus/include/PR/n_libaudio_sn_sc.h lib/libmus/src/n_libaudio_sc.h lib/libmus/src/n_libaudio_sn_sc.h
  n_alSynAllocVoice:  lib/libmus/include/PR/n_libaudio_sc.h lib/libmus/include/PR/n_libaudio_sn_sc.h lib/libmus/src/n_libaudio_sc.h lib/libmus/src/n_libaudio_sn_sc.h
  n_alSynDelete:  lib/libmus/include/PR/n_libaudio_sc.h lib/libmus/include/PR/n_libaudio_sn_sc.h lib/libmus/src/n_libaudio_sc.h lib/libmus/src/n_libaudio_sn_sc.h
  n_alSynFreeVoice:  lib/libmus/include/PR/n_libaudio_sc.h lib/libmus/include/PR/n_libaudio_sn_sc.h lib/libmus/src/n_libaudio_sc.h lib/libmus/src/n_libaudio_sn_sc.h
  n_alSynGetFXRef:  lib/libmus/include/PR/n_libaudio_sc.h lib/libmus/include/PR/n_libaudio_sn_sc.h lib/libmus/src/n_libaudio_sc.h lib/libmus/src/n_libaudio_sn_sc.h
  n_alSynGetPriority:  lib/libmus/include/PR/n_libaudio_sc.h lib/libmus/include/PR/n_libaudio_sn_sc.h lib/libmus/src/n_libaudio_sc.h lib/libmus/src/n_libaudio_sn_sc.h
  n_alSynNew:  lib/libmus/include/PR/n_libaudio_sc.h lib/libmus/include/PR/n_libaudio_sn_sc.h lib/libmus/src/n_libaudio_sc.h lib/libmus/src/n_libaudio_sn_sc.h
  n_alSynRemovePlayer:  lib/libmus/include/PR/n_libaudio_sc.h lib/libmus/include/PR/n_libaudio_sn_sc.h lib/libmus/src/n_libaudio_sc.h lib/libmus/src/n_libaudio_sn_sc.h
  n_alSynSetFXMix:  lib/libmus/include/PR/n_libaudio_sc.h lib/libmus/include/PR/n_libaudio_sn_sc.h lib/libmus/src/n_libaudio_sc.h lib/libmus/src/n_libaudio_sn_sc.h
  n_alSynSetFXParam:  lib/libmus/include/PR/n_libaudio_sc.h lib/libmus/include/PR/n_libaudio_sn_sc.h lib/libmus/src/n_libaudio_sc.h lib/libmus/src/n_libaudio_sn_sc.h
  n_alSynSetFXtype:  lib/libmus/include/PR/n_libaudio_sc.h lib/libmus/include/PR/n_libaudio_sn_sc.h lib/libmus/src/n_libaudio_sc.h lib/libmus/src/n_libaudio_sn_sc.h
  n_alSynSetPan:  lib/libmus/include/PR/n_libaudio_sc.h lib/libmus/include/PR/n_libaudio_sn_sc.h lib/libmus/src/n_libaudio_sc.h lib/libmus/src/n_libaudio_sn_sc.h
  n_alSynSetPitch:  lib/libmus/include/PR/n_libaudio_sc.h lib/libmus/include/PR/n_libaudio_sn_sc.h lib/libmus/src/n_libaudio_sc.h lib/libmus/src/n_libaudio_sn_sc.h
  n_alSynSetPriority:  lib/libmus/include/PR/n_libaudio_sc.h lib/libmus/include/PR/n_libaudio_sn_sc.h lib/libmus/src/n_libaudio_sc.h lib/libmus/src/n_libaudio_sn_sc.h
  n_alSynSetVol:  lib/libmus/include/PR/n_libaudio_sc.h lib/libmus/include/PR/n_libaudio_sn_sc.h lib/libmus/src/n_libaudio_sc.h lib/libmus/src/n_libaudio_sn_sc.h
  n_alSynStartVoice:  lib/libmus/include/PR/n_libaudio_sc.h lib/libmus/include/PR/n_libaudio_sn_sc.h lib/libmus/src/n_libaudio_sc.h lib/libmus/src/n_libaudio_sn_sc.h
  n_alSynStartVoiceParams:  lib/libmus/include/PR/n_libaudio_sc.h lib/libmus/include/PR/n_libaudio_sn_sc.h lib/libmus/src/n_libaudio_sc.h lib/libmus/src/n_libaudio_sn_sc.h
  n_alSynStopVoice:  lib/libmus/include/PR/n_libaudio_sc.h lib/libmus/include/PR/n_libaudio_sn_sc.h lib/libmus/src/n_libaudio_sc.h lib/libmus/src/n_libaudio_sn_sc.h
  osAiGetLength:  lib/libmus/src/aud_thread.c
  osAiGetStatus:  lib/libmus/src/aud_thread.c
  osAiSetFrequency:  lib/libmus/src/aud_thread.c
  osAiSetNextBuffer:  lib/libmus/src/aud_thread.c
  __osBaseCounter:  src/boot/COMMON.c src/boot/scommon.c
  __osBaseTimer:  src/boot/COMMON.c
  __osBbLastRCount:  src/boot/scommon.c
  __osBbLastVCount:  src/boot/scommon.c
  __osBbRCountWraps:  src/boot/scommon.c
  __osBbVCountWraps:  src/boot/scommon.c
  osCartRomInit:  lib/libmus/src/aud_dma.c src/boot/dmadata.c
  osContGetQuery:  src/main_segment/joy.c
  osContGetReadData:  src/main_segment/joy.c
  osContInit:  src/main_segment/joy.c
  __osContLastCmd:  src/boot/COMMON.c src/boot/scommon.c
  __osContPifRam:  src/boot/COMMON.c
  osContSetCh:  src/main_segment/joy.c
  osContStartQuery:  src/main_segment/joy.c
  osContStartReadData:  src/main_segment/joy.c
  osCreateMesgQueue:  lib/libmus/src/aud_dma.c lib/libmus/src/aud_sched.c src/boot/dmadata.c src/main_segment/audio/music_driver.c src/main_segment/bg_tasks.c src/main_segment/dm_game_main.c src/main_segment/dm_manual_main.c src/main_segment/dm_title_main.c src/main_segment/graphic.c src/main_segment/joy.c src/main_segment/main_menu.c src/main_segment/main_story.c src/main_segment/nnsched.c src/main_segment/record.c
  osCreatePiManager:  src/boot/boot_main.c
  osCreateThread:  lib/libmus/src/aud_thread.c src/boot/boot_main.c src/main_segment/bg_tasks.c src/main_segment/graphic.c src/main_segment/nnsched.c
  osCreateViManager:  src/main_segment/nnsched.c
  __osCurrentTime:  src/boot/COMMON.c src/boot/scommon.c
  osDestroyThread:  src/main_segment/bg_tasks.c
  osDriveRomInit:  lib/libmus/src/aud_dma.c
  __osEepPifRam:  src/boot/COMMON.c
  osEepromLongRead:  src/main_segment/record.c
  osEepromLongWrite:  src/main_segment/record.c
  osEepromProbe:  src/main_segment/record.c
  __osEepromTimerMsg:  src/boot/COMMON.c src/boot/scommon.c
  __osEepromTimerQ:  src/boot/COMMON.c
  __osEepromTimer:  src/boot/COMMON.c
  osEPiLinkHandle:  src/gateway/003630.c
  osEPiStartDma:  include/dmadata.h lib/libmus/src/aud_dma.c
  osEPiWriteIo:  src/gateway/003630.c
  __osEventStateTab:  src/boot/COMMON.c
  __osFinalrom:  src/boot/COMMON.c src/boot/scommon.c
  osGetCount:  lib/libmus/src/libmus_config.h lib/libmus/src/player.c src/main_segment/dm_title_main.c
  osGetTime:  src/main_segment/dm_game_main.c src/main_segment/nnsched.c
  osInitialize:  src/boot/boot_main.c
  osInvalDCache:  src/boot/dmadata.c src/main_segment/audio/music_driver.c
  __osMaxControllers:  src/boot/COMMON.c src/boot/scommon.c
  __osPiAccessQueue:  src/boot/COMMON.c
  osPiStartDma:  include/dmadata.h
  osRecvMesg:  lib/libmus/src/aud_dma.c lib/libmus/src/aud_sched.c src/boot/dmadata.c src/main_segment/audio/music_driver.c src/main_segment/bg_tasks.c src/main_segment/dm_game_main.c src/main_segment/dm_manual_main.c src/main_segment/dm_title_main.c src/main_segment/graphic.c src/main_segment/joy.c src/main_segment/main_menu.c src/main_segment/main_story.c src/main_segment/nnsched.c src/main_segment/record.c
  osScAddClient:  lib/libmus/src/aud_sched.c
  osScGetCmdQ:  lib/libmus/src/aud_sched.c
  osSendMesg:  lib/libmus/src/aud_sched.c src/main_segment/audio/music_driver.c src/main_segment/bg_tasks.c src/main_segment/graphic.c src/main_segment/nnsched.c
  osSetEventMesg:  src/main_segment/joy.c src/main_segment/nnsched.c
  osSetIntMask:  src/main_segment/nnsched.c
  osSetThreadPri:  src/main_segment/dm_game_main.c src/main_segment/main_menu.c
  osSetTimer:  src/main_segment/dm_game_main.c src/main_segment/record.c
  osSetTime:  src/main_segment/nnsched.c
  __osSiAccessQueue:  src/boot/COMMON.c
  osSpTaskStart:  src/main_segment/nnsched.c
  osSpTaskYielded:  src/main_segment/nnsched.c
  osSpTaskYield:  src/main_segment/nnsched.c
  osStartThread:  lib/libmus/src/aud_thread.c src/boot/boot_main.c src/main_segment/bg_tasks.c src/main_segment/graphic.c src/main_segment/nnsched.c
  osStopThread:  src/boot/boot_main.c
  osSyncPrintf:  lib/libmus/src/aud_dma.c lib/libmus/src/aud_thread.c lib/libmus/src/libmus_config.h lib/libmus/src/player_api.c lib/libmus/src/player.c lib/libmus/src/player_commands.c lib/libmus/src/player_fifo.c
  __osThreadSave:  src/boot/COMMON.c
  __osTimerCounter:  src/boot/COMMON.c src/boot/scommon.c
  osTvType:  lib/libmus/src/player_api.c src/main_segment/main.c
  osViBlack:  src/main_segment/graphic.c src/main_segment/nnsched.c
  osViGetCurrentFramebuffer:  src/main_segment/nnsched.c
  osViGetNextFramebuffer:  src/main_segment/nnsched.c
  __osViIntrCount:  src/boot/COMMON.c src/boot/scommon.c
  osViModeTable:  src/main_segment/nnsched.c
  osVirtualToPhysical:  lib/libmus/src/aud_dma.c lib/libmus/src/aud_thread.c src/main_segment/dm_game_main.c src/main_segment/game_etc.c src/main_segment/graphic.c src/main_segment/main_story.c
  osViSetEvent:  src/main_segment/nnsched.c
  osViSetMode:  src/main_segment/nnsched.c
  osViSetSpecialFeatures:  src/main_segment/main.c
  osViSetYScale:  src/main_segment/graphic.c
  osViSwapBuffer:  src/main_segment/nnsched.c
  osWritebackDCacheAll:  lib/libmus/src/player.c src/main_segment/audio/sound.c src/main_segment/dm_game_main.c src/main_segment/dm_manual_main.c src/main_segment/dm_title_main.c src/main_segment/main_menu.c src/main_segment/main_story.c src/main_segment/nnsched.c

## Asset loading
- src/boot/boot_data.c holds three ROM-offset tables of {SEGMENT_ROM_START, SEGMENT_ROM_END} pairs (from include/segment_symbols.h): storyRomData[] (boot_data.c:12), bgRomData[] (boot_data.c:31), _romDataTbl[] (boot_data.c:61, indexed by RomDataTblIndex).
- Boot: boot_main.c:43 DmaData_RomToRam(SEGMENT_ROM_START(dma_table), &gMainSegmentDmaInfo, ...) DMAs the dma_table segment into RAM; Main_ThreadEntry then DecompressRomToRam(mainSegmentStart, gMainSegmentDmaInfo.vram, SEGMENT_ROM_SIZE(main_segment)) (boot_main.c:50), bzeroes bss, and calls gMainSegmentDmaInfo.entry(arg) (boot_main.c:57). DmaTableEntry {vram, entry, bssStart, bssEnd} in include/dma_table.h.
- DmaData_RomToRam (src/boot/dmadata.c:35): osInvalDCache(dst,size), then a loop of 0x2000-byte blocks: DMADATA_COPY_BLOCK macro (include/dmadata.h:10 US / :15 CN) -> osPiStartDma(OS_READ, rom, vram, blk, queue) for US, osEPiStartDma(CartRomInit handle) for CN/GW; waits with osRecvMesg(OS_MESG_BLOCK). Returns dst+size (bump pointer).
- DecompressRomToRam (src/boot/util.c:10): expand_gzip(segmentRom, dst, size) (src/gzip/unzip.c) reads the gzip header + compressed chunks from ROM via DmaData_RomToRam (unzip.c:65 data_read) and inflate()s (src/gzip/inflate.c) into dst. With UNCOMPRESSED_ROM it just DMAs instead. So story/bg/anim/game segments are gzip-expanded, not DMA'd raw.
- Audio data: DmaRomToRam (src/main_segment/audio/music_driver.c:238) DMAs in 0x4000 blocks onto the music driver's dma_queue (used for ptr tables, fxbank, sound data: music_driver.c:121/135/187).
- Addressing after load: absolute pointers. Callers keep returned buffer pointers (main_story.c:517/692/894, tex_func.c:1240, char_anime.c:473, dm_game_main.c:7327) and hand them to the RSP via gSPSegment(disp, 0x05, osVirtualToPhysical(buf)) (main_story.c:725, game_etc.c:357); segment 0 is the default (main segment .data). Asset display lists are raw Gfx words in the buffers; no fixed segment-number scheme. Port implication: PI-DMA/memcpy from embedded ROM into any RAM buffer works; keep the bump-pointer return convention.

## Assembly files
- src/entry/entry.s: boot entry point; sets up the boot stack (BOOT_STACK_SIZE 0x2000) and jumps into osInitialize/main path.
- src/libkmc/mmuldi3.s: software (long long)* (long long) helper (__muldi3) for the MIPS compiler runtime.
- src/main_segment/calcsub.s: small math helpers (GameCube version has them in calcsub.c).
- src/main_segment/unused/055C40.s: unused copy/convert routine func_8007F890(u32*,u32*).
- src/rsp/aspMain.s: .incbin of the aspMain audio microcode text+data (aspMainTextStart/DataStart).
- src/rsp/f3dex2.s: .incbin of the F3DEX2 graphics microcode (gspF3DEX2_fifoTextStart/DataStart).
- src/rsp/s2dex.s: .incbin of the S2DEX microcode (gspS2DEX_fifoTextStart/DataStart).
- src/rsp/rspboot.s: .incbin of rspboot, the RSP boot microcode run before every task.

## Threads and priorities
- boot_main.c:36 osCreateThread(&sIdleThread, THREAD_ID_IDLE, Idle_ThreadEntry, NULL, STACK_TOP(sIdleStack)=0x2000, THREAD_PRI_IDLE_BOOT=1); started then stopped (osStopThread boot_main.c).
- boot_main.c:65 osCreateThread(&sMainThread, THREAD_ID_MAIN, Main_ThreadEntry, arg, sMainStack 0x2000, THREAD_PRI_MAIN=10).
- nnsched.c:95 scheduler thread: THREAD_ID_NN_SC_EVENT, nnScEventHandler, nnScStack 0x2000 (NN_SC_STACKSIZE), THREAD_PRI_NN_SC_EVENT=120.
- nnsched.c:103 audio thread: THREAD_ID_NN_SC_AUDIO, nnScExecuteAudio, nnScAudioStack 0x2000, THREAD_PRI_NN_SC_AUDIO=110.
- nnsched.c:107 graphics thread: THREAD_ID_NN_SC_GRAPHICS, nnScExecuteGraphics, nnScGraphicsStack 0x2000, THREAD_PRI_NN_SC_GRAPHICS=100.
- graphic.c:291 gfx thread: THREAD_ID_GRAPHIC, gfxproc, sGraphicStack GRAPHIC_STACK_SIZE=0x2000, THREAD_PRI_GRAPHIC=15.
- bg_tasks.c:44 bg-task manager: THREAD_ID_BG_TASK, BgTasksManager_ThreadEntry, manager->stack STACK8(...,0x1000) (bg_tasks.h:16), THREAD_PRI_BG_TASK=2.
- lib/libmus/src/aud_thread.c:125 music thread: id 3, __MusIntThreadProcess, heap-allocated stack AUDIO_STACKSIZE=0x2000 (aud_thread.c:40), priority config->thread_priority = THREAD_PRI_MUSIC_DRIVER=50 (passed via InitMusicDriver, sound.c:275).
- osSetThreadPri used at runtime: dm_game_main.c and main_menu.c (gfx thread priority tweak, dm_game_main.c:7952 sets st->graphic_thread_pri = THREAD_PRI_GRAPHIC).

## Microcode
- Graphics tasks: graphic.c:94 gfx_ucode[][2] = {F3DEX2, S2DEX} (gspF3DEX2_fifoTextStart/gspS2DEX_fifoTextStart from src/rsp/f3dex2.s, s2dex.s); scTask->list.t.ucode = gfx_ucode[arg3][0] (graphic.c:331), ucode_boot = rspbootTextStart (graphic.c:328). Tasks are queued to the NN scheduler and started by osSpTaskStart in nnsched.c:291/311 (yield restart)/346.
- Audio tasks: aud_thread.c:46-50 MICROCODE_CODE = aspMainTextStart (n_aspMainTextStart only if SUPPORT_NAUDIO; US build uses aspMain from src/rsp/aspMain.s); sched_task.ucode set aud_thread.c:159; __OsSchedDoTask (aud_sched.c:122) builds OSScTask type M_AUDTASK with ucode_boot rspbootTextStart and sends it to osScGetCmdQ; nnsched.c osSpTaskStart runs it.
- So the game actually starts: rspboot + (F3DEX2 | S2DEX) for gfx, rspboot + aspMain for audio.

## Wii build skeleton (task 02)
- Makefile.wii at repo root: powerpc-eabi-gcc, CFLAGS `-O2 -mrvl -mcpu=750 -meabi -mhard-float -DGEKKO -DTARGET_WII -I$(DEVKITPRO)/libogc/include`, LDFLAGS `-mrvl -L$(DEVKITPRO)/libogc/lib/wii -lwiiuse -lbte -logc -lm`. Objects -> build/wii/<src>.o, link -> build/wii/drmario64.elf, elf2dol -> build/wii/drmario64.dol. WII_SRCS is the source list (port/wii/main.c now). DEVKITPRO/DEVKITPPC fall back to /home/rhy/devkitpro if not exported.
- A bare gcc link (no devkit wii_rules / custom linker script) boots fine in Dolphin Wii mode and elf2dol converts it; libogc's crt + constructors handle startup. No custom crt0 needed for the skeleton.
- GX 2D ortho gotcha: GX_SetArray stride for GX_VA_POS with GX_POS_XYZ/GX_S16 is 3*sizeof(s16) (one per component), NOT 4. Using 4 garbles the quad (this exact bug produced a sheared white blob). CLR stride is 4*sizeof(u8).
- printf does NOT reach Dolphin's log here (Logger.ini OSREPORT=True but newlib stdout isn't wired to OSReport). The log only shows BOOT lines. For real logging use OSReport() directly (port exposes PORT_LOG per DESIGN). Not needed for the t02 gate.
- VIDEO_GetPreferredMode(NULL) on Dolphin Wii returns a mode whose frame dump is 780x480 (not 640x480); the white quad's on-screen position/aspect follows fbWidth/efbHeight, so don't assume 640x480 pixel coords when checking frames.
- GATE: make -f Makefile.wii && dolphin-run.sh build/wii/drmario64.dol 25 .strata/scratch/t02 && test -s last.png. last.png can be 0 bytes if the 25s timeout kills Dolphin mid-frame-write (race); re-run passes. ~1350 frames dumped in 25s.

## N64 symbol table (task 03)
- port/tools/gen_syms.py runs mips-linux-gnu-nm on build/us/drmario64.us.elf -> port/gen/n64_syms.ld (7237 syms). (a) boundary suffixes _VRAM/_VRAM_END/_ROM_START/_ROM_END/_BSS_START/_BSS_END (segment_symbols.h macros; also catches linker *_alloc_VRAM/*_noload_VRAM variants, harmless). (b) value<0x10000000 with nm type in DdBbRrTtA (drops U/N/w and size-only _HI/_LO/_CLASS_/_SIZE absolutes that are not addresses... actually A kept; size markers are A so kept too, harmless). Sorted, deduped, "NAME = 0x%08X;" lines.
- Makefile.wii: $(SYMS) rule depends on $(N64_ELF); link adds -Wl,port/gen/n64_syms.ld as a POSITIONAL extra ld script (no SECTIONS block -> devkitPPC default script untouched). Link + elf2dol succeed; DOL 306176 bytes.
- No name collisions: Wii/DOL symbols live at 0x80000000+, so low-value N64 asset syms and boundary syms never clash with libogc/newlib.

## Wii compile (task 04)
- Makefile.wii: added GAME_SRCS (src/boot, src/main_segment + audio/graphics/screen_print, src/gzip, src/buffers, src/libnustd, src/libegcs, src/dma_table, lib/libmus/src; excludes src/main_segment/unused), objs target, GAME_CFLAGS, N64_IINC.
- Makefile.wii: N64_IINC uses -Ibuild/us/bin/us (matches N64 Makefile IINC; .i4.inc/.rgba16.inc live under build/us/bin/us/...).
- Makefile.wii: objs recipe adds -I $(dir $<) -I build/us/$(dir $<) (matches N64 per-file flags; .msg.inc files live in build/us/src/<dir>/).
- Makefile.wii: GAME_CFLAGS adds -D_LANGUAGE_C -D__IS_KMC__=1 -D_MIPS_SZLONG=32 -D__USE_ISOC99 -DSCOMMON_IN_COMMON=1 -DF3DEX_GBI_2 -DNDEBUG -D_FINALROM -DBUILD_VERSION=8 (mirrors N64 BUILD/COMMON/RELEASE/GBI/C defines; fixes gSP2Triangles and ultralib type guards).
- port/include/libmus_wii_externs.h (new): extern decls for player.c statics/functions that player_commands.c/player_api.c/player_fifo.c use (they are #included into player.c on N64).
- lib/libmus/src/{player_api,player_commands,player_fifo}.c: #ifdef TARGET_WII preamble mirrors player.c's include block; also includes libmus_wii_externs.h.
- Gate: make -f Makefile.wii objs -k -j8 -> 63 objects, rc=0; N64 build still OK.
- dolphin-run.sh (Claude, 16:55): SD folder sync never ran because Dolphin is killed; the harness now extracts the SD image (Load/WiiSD.raw) with 7z into <outdir>/sd/. Works.

## SD card logging (task 02b)
- libfat is in libogc: libfat.a at ~/devkitpro/libogc/lib/wii/, fat.h at ~/devkitpro/libogc/include/. -lfat resolves with existing -L. fat.h has NO fatMkdir; use newlib mkdir() from <sys/stat.h> (routes through libfat's f_mkdir hook). Do NOT call f_mkdir directly (crashes).
- port/include/port_log.h: int port_log_init(void) (1=SD ok), void PORT_LOG(const char*fmt,...). port/wii/log.c: fatInitDefault(); mkdir("sd:/drmario",0777); fopen("sd:/drmario/log.txt","w"); PORT_LOG vfprintf+fputc('\n')+fflush; no-op if logFile NULL.
- Makefile.wii: CFLAGS += -Iport/include; LDFLAGS += -lfat before -logc; WII_SRCS += port/wii/log.c.
- KEY FIX (dolphin-run.sh): Dolphin ini needs WiiSDCardAllowWrites = True in [Core] — without it libfat fopen("w") fails and NOTHING syncs. Keys WiiSDCardEnableFolderSync/WiiSDCardSyncFolder were already correct (confirmed via strings on the dolphin-emu binary; build is dev 2609a).
- KEY FIX (main.c): the boot loop MUST exit cleanly (break at frame>=300 -> main returns -> Dolphin shuts down) for the SD folder-sync to flush. timeout SIGTERM/SIGINT does NOT flush the sync (rc=124, no file). With clean exit rc=0 and sd/drmario/log.txt appears.
- Gate: make -f Makefile.wii && dolphin-run.sh ... 25 ... && grep -q "boot ok" .../sd/drmario/log.txt -> PASS; log.txt = "drmario64 wii boot ok\nframe 1\nframe 60\nframe 300". N64 build still OK.

## Link (task 06)
- Makefile.wii: GAME_SRCS filter-out player_api.c/player_commands.c/player_fifo.c (player.c #includes them;
  separate TUs = duplicate symbols). GAME_CFLAGS adds -DUNCOMPRESSED_ROM. ROM_BIN/ROM_OBJ rules embed
  uncompressed ROM via objcopy --redefine-sym to port_rom/port_rom_end/port_rom_size. ELF rule links
  WII_OBJS + GAME_OBJS + ROM_OBJ + -Wl,port/gen/n64_syms.ld.
- MULTIPLE-DEFINITION root cause: decomp declares each uninitialized global TWICE — once in the owning .c
  (e.g. game_etc.c:46 `u8 (*attack_sprite_address)[...];`) and once in COMMON.c with the BSS macro suffix
  (COMMON.c:281 `... BSS;`). BSS macro = __attribute__((section(".bss"))) (include/attributes.h:92, CC_CHECK=0).
  N64 flag policy: regular .c -> -fcommon (type C COMMON symbols); COMMON.o/scommon.o/buffers -> -fno-common
  (strong .bss type B defs). COMMON (C) merges into B -> no clash. Wii GAME_CFLAGS had -fno-common on EVERYTHING
  -> all strong B -> 119 multiple-definition errors.
- FIX: GAME_CFLAGS uses -fcommon; target-specific rule appends -fno-common for
  $(GAME_OBJ)/src/%/COMMON.o $(GAME_OBJ)/src/%/scommon.o $(GAME_OBJ)/src/buffers/%.o (last flag wins in gcc).
- GOTCHA: make tracks timestamps, NOT compiler flags. Changing GAME_CFLAGS does NOT rebuild .o files. After any
  flag change, run `make -f Makefile.wii clean` first (or delete $(GAME_OBJ)) or stale strong-def .o files persist.
- Gate: make -f Makefile.wii (clean rebuild) -> 0 multiple-definition, 0 undefined reference; elf2dol ->
  build/wii/drmario64.dol (431680 bytes); N64 `make -j8` still "us.z64: OK". GATE PASS.
- Claude 02:00: game objects override newlib's ecvt fcvt fmod gcvt memmove modf rand random srand strcpy tanf (from libkmc/libnustd). newlib internals may call these; if odd crashes appear, rename the game copies under TARGET_WII.

## 06b2 message queues
- os_sched.c already had all MQ fns (osCreateMesgQueue/Send/Jam/Recv, side-table waiters, sched_wake_waiter preemption). Gate failed only because test_threads.c had just T4; added T1 (ping-pong 1000, seed qBA), T2 (NOBLOCK -1), T3 (H30 waits, L10 sends, counter==1 after send). All pass.

## Audio freeze (Claude, 2026-10-16) — open, worked around with -DPORT_NO_SYNTH
- With music synthesis on, the whole Wii stops at music frame ~106 (after the title loads song data segment_189D40 into
  audio_memory; that data is byte-identical to the N64 ROM). Everything stops: game threads, event pump, VI-IRQ sentinel.
- Without the music thread, or with alAudioFrame skipped (PORT_NO_SYNTH), the game runs indefinitely (2300+ frames).
- Ruled out: synth callback loop spinning (guard never hit), libmus note loop spinning (guard never hit), out-of-range EPI
  DMA (none), osAiSetFrequency (fixed: returns freq), char signedness (now -fsigned-char everywhere; KMC GCC/MIPS char is signed).
- Crash hooks (PPCExcptCurPanicFn) never report; gdb via Dolphin 2609a disconnects on this DOL (works on a tiny DOL).
- Next: bisect inside alAudioFrame after the client loop (alFxPull/envmixer/resampler/load/adpcm pulls in
  lib/ultralib/src/audio) with markers; suspect float/int conversions or table lookups indexed by a value that differs on PPC.
- Fixed by Claude today: sched_current_slot_locked idle bug, preempt-check from helper LWPs, osSendMesg from non-game LWPs,
  port_present writing the Wii XFB into the game's N64 framebuffer, PORT_LOG fsync. Old os_backend/os_thread/os_mesg deleted.
- WARNING for the renderer: os_stub.c stubs makeScaleMatrix and other calcsub helpers as no-ops; they must be real C.

## 07ca-calcsub fix (2026-10-16)
- Makefile.wii CFLAGS lacked `-Iinclude -I.` and `-DVERSION_US`: port/game/calcsub.c
  includes "calcsub.h" (in include/) which pulls libultra.h -> version.h (needs exactly one
  VERSION_* defined; port is US) and "lib/ultralib/include/ultra64.h" (root-relative, needs -I.).
  Added all three to CFLAGS; -Iinclude/-I. go last so they cannot shadow libogc/PR headers.
  No port .c file reads VERSION_* (grep), so -DVERSION_US is inert elsewhere.

## 08a gfx walker (2026-10-16)
- port/gfx/gfx.c + gfx.h: display-list walker, no drawing. Hooked in os_sp.c M_GFXTASK case
  (gfx_run_task(tp) before the SP/DP done events); Makefile.wii WII_SRCS += port/gfx/gfx.c.
- F3DEX_GBI_2 is defined in Makefile.wii CFLAGS, so gbi.h opcodes are the F3DEX2 set:
  G_DL=0xde G_ENDDL=0xdf G_MOVEWORD=0xdb G_MW_SEGMENT=0x06; G_DL p=0 call p=1 branch (gDma1p:
  w0=(op<<24)|(p<<16)|l, w1=addr). G_MOVEWORD w0=(op<<24)|(index<<16)|off, w1=data; MW_SEGMENT
  off=seg*4 so seg=(w0>>2)&0xF.
- Address resolution: addr>=0x80000000 -> direct pointer; else seg=(addr>>24)&0xF, ptr=segtable[seg]+(addr&0xFFFFFF).
- S2DEX tasks (t.ucode==gspS2DEX_fifoTextStart) walk the same way; S2DEX opcodes (gs2dex.h: G_OBJ_RECTANGLE=0x01
  G_BG_COPY=0x0a G_SELECT_DL=0x04 G_RDPHALF_0=0xe4 etc.) go through the same table.
- Walker: 18-deep DL stack, 16-entry segtable, 256-entry optable (NULL=default cmd counter), 100000 cmd cap
  (logs "runaway" if hit), stats every 300 tasks with top-8 opcodes.

## Renderer fixes (Claude, 2026-10-16 evening)
- dolphin-run.sh had been changed to GFXBackend = Null (renders nothing -> every frame black). Restored to OGL.
- GX_SetAlphaCompare takes (comp0, ref0, op, comp1, ref1); it was called as (0, GX_ALWAYS, AND, GX_ALWAYS, 0) => comp0=GX_NEVER
  => every pixel discarded. Fixed in gfx_frame_start.
- G_FILLRECT coordinates are whole pixels (gDPFillRectangle shifts integer pixels to bits 14/2); the /4 was wrong.
- port_present: game threads now call GX_SetCurrentGXThread before GX work; GX FIFO moved to MEM2; VI configured before GX.
- 08a stats showed ~40k G_NOOP (op 0) per frame: check whether lists really contain them or the walker reads past ends.

## 09a gfx TMEM and texture loads (2026-10-17)
- port/gfx/gfx_tmem.c + gfx_tmem.h add a 4 KB TMEM byte model and 8 GfxTile descriptors; no drawing/GX upload yet.
- F3DEX_GBI_2 layouts used: G_SETTIMG w0=(op<<24)|fmt<<16|siz<<8|w, w1=addr; G_SETTILE w0=(op<<24)|fmt<<16|siz<<8|l, w1=(palette<<16)|cmt<<8|maskt<<2|shiftt; G_SETTILESIZE w0=(op<<24)|cms<<10|masks<<5|shifts<<2|fmt<<16|siz<<8|l, w1=(uls<<16)|ult<<16|lrs<<8|lrt; G_LOADBLOCK w0=(op<<24)|uls<<10|ult<<2|dxt, w1=lrs<<16|lrt; G_LOADTILE w0=(op<<24)|uls<<10|ult<<2|dxt, w1=lrs<<16|lrt; G_LOADTLUT w0=(op<<24)|0x8000, w1=uls<<16|ult<<16|lrs<<8|lrt.
- LOADBLOCK uses the tile's siz for source texel size, linear source texel count from uls/lrs, and dxt*64/bits bytes per TMEM line. Odd rows swap 32-bit words within each line, matching the host test.
- LOADTLUT writes 16-bit palette entries into the upper half of TMEM starting at 0x800.
- Makefile.wii WII_SRCS now includes port/gfx/gfx_tmem.c.

## 09b gfx texconv (2026-10-17)
- port/gfx/gfx_texconv.c + gfx_texconv.h: pure decoder gfx_decode_tile(const uint8_t *tmem, const GfxTile *t, int w, int h, uint8_t *out_rgba8, uint32_t otm_h). No GX upload yet; reads the 09a TMEM model, writes row-major RGBA8.
- Signature carries otm_h (other-mode-H word) as a 6th param so TEXTLUT is a parameter; tlut=(otm_h>>G_MDSFT_TEXTLUT)&3 selects RGBA16 (5551) vs IA16 (12I+4A) for CI/TLUT palette entries. gfx_st.otm_h is NOT readable here because gfx_xform.c is not on the host-test link line.
- Formats decoded: RGBA16=5551, RGBA32=8888, IA4=3I+1A, IA8=4+4, IA16=12I+4A, I4/I8=intensity opaque alpha, CI4/CI8=TLUT index (palette 16-bit at 0x800+(palette+idx)*2). YUV/AI left transparent black (unused by this game).
- KEY: task 09b spec says t->line is in 64-bit WORDS -> decode row stride = line*8 bytes. NOTE this conflicts with gfx_tmem.c LOADTILE which uses line*4 (32-bit words); the host test is hand-built to match the 09b spec, not gfx_tmem.c's LOADTILE.
- KEY: LOADBLOCK leaves odd rows with adjacent 32-bit words swapped; undone on read by addr^4 on odd rows (swaps the two longs within each 8-byte group).
- Expansions: exp5(v)=(v<<3)|(v>>2), exp3(v)=(v<<5)|(v<<2)|(v>>1), exp4(v)=v*0x11.
- GOTCHA: gfx_tmem.c declares extern gfx_mem() under HOST_TEST; the texconv host test must supply a gfx_mem() shim (returns NULL here since TMEM is built by hand) or the link fails "undefined reference to gfx_mem".
- Makefile.wii WII_SRCS now includes port/gfx/gfx_texconv.c. GATE (make + gcc link of test+texconv+tmem + run) prints GFX_TEXCONV PASS.

## Combiner (Claude rewrote port/gfx/gfx_cc.c, 2026-10-17)
- w0 mux is 24 bits (mask 0xFFFFFF); each N64 slot (A/B/C/D, colour and alpha) has its own selector table (see comments).
- TEV registers: PREV = partials and final output (GX reads PREV!), REG1 = cycle-1 result (COMBINED in cycle 2),
  REG2 = environment colour (GX_CC_C2/A2), KONST K0 = primitive colour (KCSEL_K0 / K0_A).
- gfx_cc_apply_tex(textured): textured draws pass 1 (texrects are textured even when G_TEXTURE is off); untextured 0.
  COPY-mode texrects use 1 stage REPLACE and call gfx_cc_invalidate(). gfx_frame_start invalidates too.
- Prim/env colours are re-sent on every apply (fades change them every frame); only the stage program is cached.

## Textures (Claude rewrote gfx_tmem.c loads and gfx_texconv.c, 2026-10-17)
- TMEM model: tile.tmem/line in 64-bit words; row y at tmem*8 + y*line*8; odd rows XOR 4 (LOADBLOCK via the dxt
  counter bit 11, LOADTILE by destination row parity); TLUT entries quadrupled: entry i at 0x800 + i*8.
- Tile sizes (uls/lrs) are 10.2: texture width = ((lrs-uls)>>2)+1 (gfx_tex.c used lrs-uls+1 -> 4x magnified texels).
- IA16 is 8-bit I + 8-bit A (not 12+4). I4/I8 use the intensity for alpha too.
- Result: dumped textures are the real title art; frame shows "PRESS ANY BUTTON" correctly. Background art and the
  text's transparency are still missing (S2DEX bg rects = 10d, blending = 10c).
- Tests run at ~18 fps in the harness because Dolphin renders with llvmpipe under Xvfb; not a game problem.

## Render state / other mode (task 10c, gfx_rm.c, 2026-10-17)
- New file port/gfx/gfx_rm.c: gfx_rm_apply() called at top of every draw (gfx_draw_tri, gfx_fillrect in gfx_gx.c;
  gfx_tex_draw_tri, texrect in gfx_tex.c). Reads gfx_st.otm_l/otm_h (F3DEX_GBI_2 split, othm_l/othm_h handlers in
  gfx_xform.c store w1 verbatim).
- otm_l bits: AC 0-1 (0=NONE->GX_ALWAYS, 1=THRESHOLD->GX_GEQUAL vs blend alpha, 3=DITHER->GX_GEQUAL 0x80),
  ZS bit2, AA_EN 0x8, Z_CMP 0x10, Z_UPD 0x20, IM_RD 0x40, CLR_ON_CVG 0x80, CVG_DST 8-9, ZMODE 10-11.
- otm_h (right-justified 20-bit): CVG_X_ALPHA 0x1000, ALPHA_CVG_SEL 0x2000, FORCE_BL 0x4000, blender 16-31,
  cycle type 20-21 (0=1CYC,1=2CYC,2=COPY,3=FILL).
- Blend: FORCE_BL and cycle!=FILL/COPY -> GX_BM_BLEND/SRCALPHA/INVSRCALPHA/LO_CLEAR else GX_BM_NONE; FILL forces NONE.
- GX_SetAlphaCompare arg order is (comp0, ref0, op, comp1, ref1); GX_SetZMode(enable, GX_LEQUAL, update).
- gfx_tex.c had a bug: cycle type read from (otm_l >> 20) & 3 - cycle type lives in otm_h; fixed to otm_h.
- Log: "RM %08x" once per distinct otm_l (cap 64). Gate run: 4 distinct values (0, 1, 0x504240, 0x0f0a4000);
  imgmax 1062 colors >= 64.

## Geometry (Claude, 2026-10-17)
- gfx_mem() on Wii ignored segments (mapped 0x05xxxxxx to 0x80xxxxxx) -> vertices/matrices from asset segments were
  garbage. It now calls gfx_resolve().
- N64 uses ROW vectors: clip = v * (MV * P), translation in row 3; G_MTX multiply is new = M * current; mtx_mul must
  not alias its output. Perspective divide by w and viewport y flip added in xf_vtx.
- Result: textured 3D intro geometry appears, but triangles crossing w <= 0 explode (no clipping yet) -> task 10e.

## gu* name clash (Claude, 2026-10-17) -> TITLE SCREEN RENDERS
- libogc defines guOrtho/guPerspective/... with different signatures (float Mtx44); the game's calls linked to those, and
  guTranslate/guMtxF2L/guRotateRPYF/sins were no-op stubs in os_stub.c. Now: port/include/n64_gu_rename.h is
  force-included into GAME_CFLAGS (renames every N64 gu* to n64_gu*), and lib/ultralib/src/gu/{mtxutil,ortho,...}.c are
  compiled as game sources. Stubs removed.
- Result: the full title screen renders (logo, background, sprites, text). Remaining: alpha (boxes around sprites) = 10f.

## Render modes (Claude, 2026-10-17)
- G_SETOTHERMODE_H/L set ONE field: w0 = op | (32-shift-len)<<8 | (len-1); merge with a mask (they used to overwrite the
  whole word). G_RDPSETOTHERMODE (0xEF) sets both words. F3DEX2 G_GEOMETRYMODE (0xD9) is one opcode: geo = (geo & w0) | w1
  (set/clear handlers used to collide on the same table slot).
- gfx_rm.c reads FORCE_BL/CVG bits from othermode L; translucent only for blender P*A + MEM*(1-A).
- Result: title screen correct (scratch/title-ok.png) except the black box behind "PRESS ANY BUTTON".

## 10g gfx textbox (2026-10-17)
- Black box behind "PRESS ANY BUTTON" was already fixed by gfx_rm.c coverage-from-alpha (G_AC_THRESHOLD + CVG_X_ALPHA/ALPHA_CVG_SEL -> GX alpha compare GREATER 0 discards alpha-0 texels; blend only for P*A+MEM*(1-A)). Pixel (232,345) in framedump_9xx = (255,206,156), sum 617 > 60.
- Real cause of the failed gate: the task GATE was multi-line, but run-step.sh extracts it with `grep -m1 "^GATE:"` (one line only), so GATECMD ended in an unterminated `"` -> `bash -c` "unexpected EOF". Fix: collapse the GATE to ONE line with balanced quotes (no rendering change needed).
- GATE now passes: make -f Makefile.wii (no-op) + dolphin-run 60s + PIL pixel check -> rc 0.
- Confirmed 2026-10-17: ran the one-line GATE myself; framedump_900/930 pixel (232,345) = (255,206,156), sum 617 > 60. Gate exit 0.

## 12a input real headers (2026-10-18)
- Moved ALL libogc WPAD/PAD use out of os_cont.c into port/wii/pad_wii.c (real <gccore.h>/<wiiuse/wpad.h>); os_cont.c now only calls port_pad_scan(PortPad[4]) via port/include/pad_wii.h. Deleted the hand-declared WPAD/PAD prototypes, the wrong button enums, and the ABI-risk expansion_t structs from os_cont.c.
- Real libogc API gotchas (verified in wpad.h/wiiuse.h): WPAD_Probe(s32 chan, u32 *type) -> use `unsigned int ptype`, probe enum is WPAD_EXP_NONE/NUNCHUK/CLASSIC/... (NOT WPAD_PROBE_TYPE_*). WPAD_Expansion(int, struct expansion_t*) returns VOID (not int) -> call it then test exp.type==WPAD_EXP_CLASSIC. expansion_t.classic is struct classic_ctrl_t; .btns is u32 with classic bits in HIGH 16 (WPAD_CLASSIC_* = bit<<16), so port_map_classic is fed (btns>>16) and uses low-16 CLASSIC_* masks.
- pad_wii.c compiles in TWO modes: host test (-DPAD_HOST_TEST, plain gcc, fallback #defines with the REAL WPAD/PAD bit values, libogc parts behind #ifndef PAD_HOST_TEST) and Wii build (real headers). Pure mapping helpers port_map_wiimote_sideways/port_map_classic/port_map_gc compiled in both.
- WPAD_Init/PAD_Init now done once inside port_pad_scan (static flag); osContInit no longer calls them. main.c's WPAD_Init() left alone (double WPAD_Init was already happening pre-task and is fine).
- GATE passed: host test prints "pad map OK"; grep gate (no WPAD_BUTTON_UP=0x / enum{WPAD in os_cont.c) clean; make -f Makefile.wii ok; dolphin 60s; imgmax 106318 colors >= 128.

## 12c event ring + timer races (2026-10-18)
- irqlock.c already existed (port_irq_disable/restore wrapping IRQ_Disable/IRQ_Restore, in Makefile.wii line 100); port_post_event already used it. Only fixed the stale header comment (it claimed no IRQ guard + one producer).
- port_post_event: ring-full now increments g_ring_drops; PORT_LOG("event ring full drops=%u") fires on the 1st and every 64th drop, OUTSIDE the IRQ-disabled region (log after port_irq_restore).
- counter_ticks_now: exact ratio gettime()*125ULL/162ULL (46.875/60.75 MHz = 125/162); old two-step division lost precision.
- timer_body: IRQ-guarded scan; collect due timers into local EventEntry due[MAX_TIMERS] then post AFTER port_irq_restore (never port_post_event/usleep while IRQs disabled). Periodic: expiry += interval, and if expiry<=now set expiry=now+interval (kills drift). When !has (no timer armed) block on LWP_SemWait(g_timer_sem) instead of usleep; keep non-blocking drain when armed.
- osSetTimer/osStopTimer: g_timers find+write now wrapped in port_irq_disable/restore. osSetTimer full table -> PORT_LOG("timer table full").
- GATE passed: WII_SELFTEST build links, dolphin 30s log has "SELFTEST PASS" (T1-T7 incl T6b periodic 18-22 fires, T7 osGetTime 46.875e6 +-5%), full build + 60s dolphin + imgmax 115679 colors >= 128.

## 12d cheap logging + clean boot (2026-10-18)
- log.c: mutex now LWP_MutexInit'd inside port_log_init (lazy init raced when two threads logged first). setvbuf(logFile, logBuf[16384], 'F'). fflush+fsync only when 32 lines pending, 1s of TBL ticks (60.75e6, same base as os_events gettime) passed, or line starts "CRASH". Added port_log_flush() (forces sync), called from port_crash_report before parking. GetClock() is NOT in this libogc's headers -> used the inline mftbu/mftb TBL read (log_gettime).
- Per-frame/periodic debug logs now opt-in under #ifdef PORT_LOG_VERBOSE: main.c "present f=", os_events.c "pump cb=", os_cont.c "scan gf=", main_story.c "demo_title", joy.c "joycore ENTER"+"joy0 button=" (kept inside their #ifdef TARGET_WII).
- sentinel.c body + port_sentinel_start() call (main.c) + port_sentinel_sample() call (os_events.c) all under #ifdef PORT_SENTINEL (off by default -> no sentinel.txt, no per-half-second LWP file churn).
- Removed dead XFB colour-toggle code: g_sent_xfb/g_sent_pitch/g_sent_toggle, bt_dc_flush, the paint block in vi_retrace_callback, port_vi_sentinel_set_xfbs (os_events.c) and the if(0) call block (main.c).
- main.c allocated both XFBs twice (main + port_gx_setup) -> leaked ~1.2MB MEM1 and VIDEO_Configure ran twice. Now allocate once in main() before port_gx_setup (which reuses them); one VIDEO_Configure. SYS_AllocateFramebuffer count in main.c == 2.
- GATE passed: make -f Makefile.wii rc0; dolphin 60s rc0; imgmax 11542 colors >=128; log has 0 "present f=/scan gf=/demo_title"; no sentinel.txt; SYS_AllocateFramebuffer==2.

## 12f2 self-test regression (Claude, 2026-10-18)
- Cause: 12d wrapped only the BODY of `if (g_dbg_pump <= 5u || ...)` in `#ifdef PORT_LOG_VERBOSE` in event_pump_body, so the
  `if` captured the next statement - the event-ring drain loop. Events were delivered on the first 5 wakes, then every
  60th. Symptoms: T5 got 14 VI messages/s instead of ~60, T6 timers late, game threads fed in 1-second batches.
- Found with a RAM-only trace (no SD logging, which changes timing): T5 WAITING + registered as waiter + its queue empty
  after the pump ran -> the pump was not draining. Fixed by moving the #ifdef above the if. Self-test T1-T6 pass again.
- log.c only syncs when another line is logged, so trailing lines are lost -> 12f3 adds a periodic flusher.
- 12f3: flusher LWP at pri 10 starves T7 (pri 20) because usleep at higher priority doesn't properly suspend the LWP.
  Fix: flusher priority 90 (same as timer LWP, lowest priority). Unconditional fflush+fsync in flusher body every 1s
  commits the one-flush-lag content. Gate passes: SELFTEST PASS + T7 both present, imgmax 115679 colors.

## Task 12g (N64 build guard)
- Unguarded port lines found by scanning git diff src lib include: PORT_LOG("drpre2") in music_driver.c and unguarded #include "port_log.h" in sound.c, dm_title_main.c, nnsched.c. All now wrapped in #ifdef TARGET_WII.
- UNCOMPRESSED_ROM / PORT_NO_SYNTH / PORT_NO_MUSIC guards are Wii-only defines in Makefile.wii, so they keep the N64 build intact.
- Makefile.wii CFLAGS now include -fno-strict-aliasing -fno-common (line 94 already appended -fno-common to some GAME_CFLAGS; harmless duplicate).
- GATE passed: N64 make prints ": OK", Wii build OK, dolphin 60s dump has 115679 colors.

## Task 13 (START leaves title - audio/id=18 deadlock)
- Root cause: with -DPORT_NO_SYNTH, aud_thread.c:193 sets commands=0, so alAudioFrame is skipped.
  alAudioFrame is what drives __MusIntMain (player.c:155), the per-frame channel update that
  decrements cp->stopping and clears cp->pdata (player.c:199 Fstop). With it skipped, MusAsk()
  (player_api.c:457, counts channels with cp->pdata!=NULL) never reaches 0, so dm_audio_is_stopped()
  (sound.c:344) is permanently false. MAIN's post-title fade-out loop (dm_title_main.c:159)
  `while (!dm_audio_is_stopped() || title_fade_count<0xFF)` never exits -> main_no never updates.
  id=18 (nnScExecuteAudio) run_total=0 because NnSchedDoTask (aud_thread.c:202) is also gated by commands!=0.
- Fix: sound.c dm_audio_is_stopped() returns true under PORT_NO_SYNTH (nothing is synthesized, so
  audio is effectively stopped). This unblocks the fade-out loop; the loop still bounded by title_fade_count.

## 14b1 freeze (Claude, 2026-10-18) - FIXED
- Cause: gfx_fillrect drew a POS+CLR0 quad while the vertex descriptor still had TEX0 from a previous textured draw ->
  GP desync -> GX_DrawDone in port_present never returned (first fill rect after a textured draw = title fade-out).
  Fix: gfx_set_textured(0) + TEV order reset before the quad. Attract demo now reaches main_no=5 and back to 3.
- Also fixed: Heap_bufferp was UNK_SIZE (1 byte) on Wii -> heap overran gfx_freebuf etc. Now 0xE09F0 (N64 layout).
- Also fixed: boot race (g_status[0] not present before first scan -> MAIN_CONT_ERROR).
- Reverted task 13's priority "aging" in os_sched.c (broke strict N64 priority, self-test T4); not needed now.
- Harness: PNG compression level 1 -> Dolphin runs ~real time (was ~55%). git guard in .strata/bin/git.
- The garbage bytes at the end of SD logs are an artifact of Dolphin being killed mid-write, not memory corruption.

## Task 14c (EEPROM saves on SD)
- Replaced os_stub.c EEPROM stubs (Probe/LongRead/LongWrite -> -1) with port/ultra/os_eeprom.c: 4 Kbit EEPROM (EEPROM_TYPE_4K=0x01, 64*8=512 bytes = CACHE_SIZE 0x200) in RAM, persisted to sd:/drmario/eeprom.bin.
- record.c save-check: EepRom_ReadAll compares header to "DM64"; blank must NOT match -> EepRomErr_NotInit -> main1x.c:190 EepRom_InitFirst -> EepRom_WriteAll -> osEepromLongWrite. So a missing file is filled 0xFF (never spells "DM64"); both 0x00/0xFF init a fresh save, picked 0xFF (conventional blank cart).
- Flow at boot: Probe (loads file once, logs "eeprom: loaded|created") -> LongRead(0,cache,512) -> LongWrite(block,8) per differing block; each write fwrite's the whole 512-byte RAM snapshot (fflush+fsync+fclose) so the file is always a faithful 512-byte image.
- EEPROM access is single-threaded in this game (record.c via B_800F3E38), so no mutex; file I/O done inline per the task hint. Added port/ultra/os_eeprom.c to WII_SRCS.
- GATE passed: make rc0 (REGRESSION1 empty), dolphin 60s, imgmax 115679>=128, eeprom.bin=512 bytes, log has "eeprom: created".

## Task 15a (n_aspMain interpreter, port/audio/naudio.c)
- Wrote port/audio/naudio.h + naudio.c: `void naudio_run(const uint64_t *cmds, int n)`; each cmd is w0<<32|w1. State: dmem[4096] (big-endian int16), seg[16], codebook[32], loop_addr, dram_base, setbuff state (sb_flag/in/out/count), vol state (cur/tgt/rate). naudio_set_dram_base() lets host tests use arena offsets (raddr = dram_base + seg[0] + addr).
- Bit layouts confirmed from n_abi.h (n_audio) + abi.h (shared a* macros); see task notes. ADPCM nibble order = HIGH nibble first per byte; scale=0 -> sample=nibble (matches (nib<<12)>>(12-scale) with arithmetic shift).
- RESAMPLE: 4-tap polyphase, pitch 1.15 fixed (0x8000=1.0), state = 4 history samples + frac in RDRAM; at pitch 0x8000 output[n]==input[n-3] (4-tap delay). MIXER saturates to [-32768,32767]. INTERLEAVE uses a temp buffer (in-place overwrite corrupts). DMEMMOVE uses memmove (overlapping).
- GOTCHA: test command-count arg to naudio_run must equal the array length (mixer block had 4 cmds but passed 3 -> savebuff skipped -> stale arena read). ADPCM test frame must encode nibbles high-first (0x01,0x23,0x45,0x67,... for samples 0..7), not 0x01,0x02,0x03.
- GATE passed: gcc test prints "naudio tests OK" (8/8) + make -f Makefile.wii rc0.

## 15b post-mortem (Claude, 2026-10-21)
- The "audio freeze" was never a scheduler deadlock. With synthesis on, the RSP helper LWP crashes on the first audio
  task (CRASH pc=0 lr=0, r1 inside g_rsp_stack): naudio writes outside its buffers (raddr ignores that Acmd addresses
  are direct Wii pointers). Dolphin then raised a modal panic dialog, which froze the headless emulator - so logs just
  stopped and everything looked hung. Harness now sets UsePanicHandlers=False and logs POWERPC/MEMMAP: look for
  "ISI exception"/"DSI exception"/"Invalid read" in <outdir>/dolphin.log FIRST when a run seems frozen.
- 15b's AESND output code and PORT_NO_SYNTH removal stay; 15c fixes the crash.
- 15c2 (Claude): audio tasks run synchronously in nnScExecuteAudio via port_audio_task_run (os_rsp.c); no yield/rspMQ.
  Reverted the post-15b debugging rework (DOL-thread present pump at ~58k presents/s, priority aging re-added, log
  close/reopen + 38 s exit watchdog, boot markers) by restoring those files from snapshots/15b-audio-output-10181934.tgz;
  pre-revert state saved as snapshots/pre-revert-10210540.tgz. Kept: os_stub.c AESND output, naudio fixes,
  osSetIntMask without preempt, sound.c skipping the MIPS self-patch in dm_audio_set_stereo (mono option is a TODO).
- The black frame at ~240 is the normal logo->title fade; the title only continues once dm_audio_is_stopped() is true,
  so ANY audio stall shows up as "stuck at main_no=3 on a dark screen".
- 15c3 (Claude): title hang with synthesis on = AESND voice never playing in some builds (layout dependent; Dolphin
  picks its AESnd HLE). osAiGetLength then grows, libmus frame_samples goes negative (aud_samples.c), alAudioFrame
  returns 0 commands, music never stops, title waits forever. Found with -DPORT_SCHED_DUMP2 (thread dump every 10 s
  from the log flusher + nnsched client walk + "aud: zero cmds" log). 15c4 replaces AESND with AI DMA (AUDIO_*).
- gfx.c: the "unhandled op in main_no=5" log fired every task (opcount resets per task) and wrote ~1M lines to SD,
  freezing video. Now once per opcode per run. Never log per frame/task without a once-only guard.
- main_no is defined in main.c, COMMON.c and scommon.c (one is the real one); gfx.c's extern read 5 while the main loop
  logged 6 - check which definition gfx.c binds to before trusting it.
- 15c4 (Claude): replaced AESND with Wii AI DMA (AUDIO_*) in os_stub.c; GATE passes (frames=4200, underruns=3, imgmax 115679).
  Linkable cache-flush primitive is PPCDCacheFlushAsync (the NoSync variant); PPCDCacheFlush is NOT linkable. ai_dc_flush
  calls PPCDCacheFlushAsync(buf,size) then __asm__ volatile("sync"). Ring: 4 slots x 8192B, 32B aligned; 2048B silence.
  ai_dma_cb drops playing head, feeds next queued via AUDIO_InitDMA (never StartDMA again), else silence+underrun++.
  Guard ring with ai_lock/ai_unlock (port_irq_disable/restore), never a mutex, never PORT_LOG inside the callback.
  osAiGetLength = AUDIO_GetDMABytesLeft() + queued bytes. osAiSetNextBuffer: count>=4 -> -1, else memcpy+pad32+DCFlush+enqueue.
  Callback fires ~once/frame (cb~calls), count stays 1, ring drains. Earlier no-diag run showed frames=1 (layout-dependent
  Dolphin AI DMA HLE); removing diagnostics kept it working (frames=4200). Disk was 99% full; cleaned old scratch dirs.

## Task 16 (480p progressive + 16:9 presentation)
- port/wii/main.c: added `static GXRModeObj *port_pick_mode(void)` used by BOTH main() and port_gx_setup() (replaced the two separate VIDEO_GetPreferredMode(NULL) calls). Picks &TVNtsc480Prog when CONF_GetProgressiveScan()>0 && VIDEO_HaveComponentCable() (PAL60 -> &TVEurgb60Hz480Prog via CONF_GetEuRGB60()>0), else falls back to VIDEO_GetPreferredMode(NULL). Logs once: "video: mode=%s w=%d h=%d wide=%d".
- 16:9 trick: when CONF_GetAspectRatio()==CONF_ASPECT_16_9, set mode->viWidth=678 and mode->viXOrigin=(VI_MAX_WIDTH_NTSC-678)/2 BEFORE VIDEO_Configure. The VI squeezes the 640-wide XFB into a 678-pixel window so a 16:9 TV shows the 4:3 game with correct proportions (no stretched sprites). EFB/XFB stay 640 wide (fbWidth/efbHeight/xfbHeight untouched), so the renderer's 320x240 -> 640x480 scale is unchanged.
- Added #include <ogc/conf.h> (CONF_Get* + CONF_ASPECT_16_9). VIDEO_HaveComponentCable + VI_MAX_WIDTH_NTSC come from gccore.h (video.h/video_types.h).
- FUTURE "wide backgrounds" option (NOT implemented): the 16:9 trick only pillarboxes the whole 4:3 image; it does not reveal more of the world. A true wide-background mode would widen ONLY the background layer (render the bg pass at a wider horizontal FOV / wider ortho X range and sample wider bg textures) while keeping the 4:3 gameplay field (pills, board, sprites) unscaled, then composite onto the same 640-wide EFB. That needs per-layer projection changes in the renderer (gfx_xform.c ortho/perspective X extents), not a VI-mode tweak.

## 15d text corruption (menu fade)
- Root cause: OSTask data_ptr (display list / microcode) and DMA destinations were cast straight to pointers without resolving. A physical address 0x00xxxxxx read as a display-list pointer reads Wii PPC code as gfx commands; those bogus ops write into .text (0x80009518 = tm_loadtile). Canary fired "after gfx task 211".
- Fix: every address->pointer site now maps physical 0x00-0x017FFFFF -> 0x80000000|addr, keeps 0x80/0x90, resolves 0x0Sxxxxxx via seg table, and REJECTS (log once, skip) any resolved address inside [__text_start,__etext). Sites: gfx_resolve (gfx.c), resolve_rDRAM (naudio.c), rsp_resolve_data (os_rsp.c data_ptr), dma_resolve_dst (os_stub.c osPiStartDma/osEPiStartDma).
- Linker: --defsym=__text_start=0x80003f00 --defsym=__etext=0x800c1f00 on the Wii link line (gcn.ld PROVIDE(__etext) is inert because n64_syms.ld is the positional -Wl script). C refs use extern char __text_start[]/__etext[] (absolute symbols).
- GATE rc0: no CRASH, no Unknown instruction, main_no=5 reached, 120+ framedumps, imgmax 114112 colors.
- (Claude) The hardcoded --defsym __etext=0x800c1f00 was already 0x1e0 short of the real .text end (0x800c20e0). Replaced
  by port/gen/wii_text.ld (`__text_start = ADDR(.init); __etext = ADDR(.text)+SIZEOF(.text);`) on the link line.
  Never hardcode section addresses: they move with every code change.
- Open: naudio logs once "addr 00010000 lands in .text, rejected" (physical 0x10000 = N64 RAM code area). Find which
  Acmd/segment produces it; if it is a real buffer, the segment base for that command is wrong.

## Task 17 (Claude, 2026-10-21)
- `make -f Makefile.wii hbc` -> build/wii/apps/drmario64/{boot.dol,meta.xml,icon.png} (icon from port/tools/make_icon.py,
  no copyrighted art). boot.dol embeds the ROM built from the user's baserom: personal use only, never distribute.
- ISO parked: wit composes only from an extracted-disc layout and needs sys/apploader.img + boot.bin + bi2.bin (+ ticket,
  tmd, cert) from a real disc; it does not generate an apploader. Do NOT hand-craft FST/apploader. Options for rhY: use
  the sys/ files of a Wii disc he owns as template, or ship as HBC package / boot the DOL in Dolphin directly.
- Rule reminder: the 17 run wrote a 272k-line wit disassembly to /tmp/opencode (RAM). Never write to /tmp.
- 18 (Claude, 10:50): gfx_rtt_flush read rtt_stage right after GX_CopyTex WITHOUT DCInvalidateRange -> CPU saw stale
  cache (zeros) -> background still black although "rtt: flushed" logged. Added GX_DrawDone + DCInvalidateRange(rtt_stage)
  before the RGBA8->RGBA5551 conversion. Rule: after ANY GPU->RAM copy, invalidate the CPU cache before reading it.
- 18 (Claude): two more facts. (a) The hand-written prototype GX_SetTexCopySrc(float,float,...) was wrong (libogc: u16):
  floats go in FP registers -> copy region garbage. Fixed. NEVER hand-declare libogc functions with guessed types; copy
  the exact prototype from ~/devkitpro/libogc/include/ogc/*.h. (b) Dolphin default EFBToTextureEnable=True never writes
  EFB copies to RAM, so CPU readback of GX_CopyTex output is all zeros in default Dolphin -> 18b keeps it on the GPU.
  The redraw (CopyTexBlock16, COPY mode) is broken independently -> 18a.
- 18a (Strata fix, noted by Claude): (1) COPY cycle was tested on otm_l; cycle type lives in othermode H bits 20-21
  (otm_h >> 20 & 3 == 2). Every COPY-mode texrect in the game was drawn as 1-cycle before. (2) dtdy is NOT /4 in COPY
  mode (only dsdx). (3) GX textures padded to power-of-two sizes; texcoords divide by the padded size (bind_gx_w/h).
  Test pattern (-DPORT_RTT_TESTPATTERN) proves the CopyTexBlock16 redraw. Remaining: 1-pixel black seams between the
  6-row strips -> in COPY and FILL mode the N64 rect's lower-right edge is INCLUSIVE (draw yh+1 / xh+1).
- 18b (Claude, 2026-10-21): GPU-alias flush (GX_CopyTex -> MEM2 RGB565 alias + bind alias_obj + texcoords in alias
  space) compiles; both N64 and Wii builds OK. BUT the 90s harness run never exercised it: the off-screen cimg
  detector (`!in_fb && width in [320,512]`) did NOT fire -> only the two main-FB cimgs logged (w=320, alternating
  0x80d561ec/0x80d7b9ec), so rtt_active stayed 0 and gfx_rtt_flush / "rtt: gpu-alias" never ran. The 93-98%
  non-black framedumps (30..420) are the game's OWN framebuffer draw, NOT the alias path -> alias path UNVERIFIED.
- 18b (Claude): the scratch/t18b DOL logged "cimg dbg" while current source logs "cimg all" -> that DOL predates the
  latest gfx_xform.c edits (stale binary vs source). A fresh rebuild + rerun is required before trusting the run.
- 18b (Claude): ENV BLOCKER for the GATE. 90s harness dumps only ~420 emulator frames (OGL backend + per-frame PNG
  encode is ~4.7 fps) and Xvfb dies mid-run ("The X11 connection broke (error 1). Did the X11 server die?"). framedump_3000.png
  is unreachable here; the GATE as written cannot pass in this environment. Need either a longer SECS, a fix to the
  off-screen cimg detector, or owner guidance.
- 18b (Claude): UNVERIFIED risk -> GX_InitTexObj(&alias_obj, ..., rtt_w*2=656, 480, GX_TF_RGB565, ...) uses NON-power-of-two
  dims; GX tex engine normally needs POT. If the alias path ever binds, 656x480 may fail on real GX / Dolphin OGL. Consider
  POT padding (1024x512) + adjust texcoord divisors if binding is reached.
- 19 (Claude, 2026-10-21): three rendering defects fixed.
  A (black seams): COPY/FILL texrect+fillrect lower-right edge is INCLUSIVE on N64 -> draw yh+1/xh+1 for cycle type COPY/FILL only (1/2-cycle stay exclusive). Fixed in gfx_tex.c (texrect) and gfx_gx.c (fillrect). blackrows=0.
  B (black gauge box): the empty part of the Virus-level gauge was drawn solid black; resolved by the COPY/FILL inclusive-edge fix + correct TLUT decode. box mean ~190 (>30).
  C (grey glove): glove is CI8 (fmt=2,siz=1) with an IA8 TLUT (G_MDSA_TEXTLUT=2, G_TT_IA8). put_entry16 in gfx_texconv.c only decoded tlut==3 (IA16) as IA; tlut==2 (IA8) fell through to RGBA16 decode, so alpha-bearing TLUT entries became opaque flat grey (172,172,172). IA8 and IA16 share the same layout (I=high byte, A=low byte), so decode tlut==2||tlut==3 as IA. Also added tlut mode to the CI texture-cache key in gfx_tex.c so different TEXTLUT modes never collide. glove mean now 155 (background shows through), stddev 0 -> not flat grey 172. GATE PASS.
- 19 (Claude): REVERTED the put_entry16 change (tlut==2 decoded as IA). gbi.h: G_TT_NONE=0, G_TT_RGBA16=2<<14,
  G_TT_IA16=3<<14 - there is NO IA8 TLUT mode. The change turned every RGBA16-palette texture greyscale (the gate
  passed only because the glove grey changed 172->155). Always check enum values in gbi.h before "fixing" a decoder.

- 19b transparent texels: missing GX_SetZCompLoc(GX_FALSE) when N64 alpha compare is active made alpha compare test interpolated vertex alpha instead of TEV/texture output alpha, so alpha-0 texels drew opaque. Fixed in gfx_rm.c by setting ZCompLoc false for alpha-compare draws and true otherwise.
- 19b clues (Claude): option plates (LOW/MED/HI, FEVER..OFF) are CI8 + G_TT_RGBA16 texrects, 1-cycle (cyc=0),
  combiner fc119623/552eff7f = colour (TEXEL0-ENV)*PRIM+ENV, alpha TEXEL0*PRIM; prim/env animate per frame (highlight).
  gfx_cc's 3-stage TEV mapping of (A-B)*C+D is mathematically right. The plates were COLOURED (lavender/purple/red) in the
  15d run and turned black after 18a's copy-cycle fix (otm_h) -> compare how these draws were classified/handled before
  and after that change (cycle type, texture decode path, POT padding / texcoord divisor bind_gx_w for CI8 textures).
- 20 (Claude): tile origin fix (texrect/tri sample s - tile.uls/4) made tiled images correct (magnifier, labels, title).
  But Strata also "fixed" gfx_tmem.c to satisfy an out-of-date host test and garbled every CI/palette texture. The CODE
  was right: LOADBLOCK odd rows use the RDP counter (+= dxt per 64-bit word, odd = bit 11; dxt = CALC_DXT = 2048/words),
  and TMEM stores TLUT entries quadricated (entry i at 0x800 + i*8). Test fixed (dxt 512, quadricated TLUT checks).
  Rule: when a host test and visually-correct output disagree, suspect the TEST and check against gbi.h / RDP docs.
- Remaining small defects (Claude, vs rhY's screenshots): black vertical bar right of the title logo (near (R));
  rough "(C)2001 Nintendo" text; dark margins at the bottom-left of the 1P score panel; menu option plates black (19b).

## Task 21: multi-pad + multi-port input script
- PAD_ScanPads() returns a BITMASK (bit i = port i), not a count; Wiimotes are probed per channel via WPAD_Probe(i,&type)==WPAD_ERR_NONE (s32 WPAD_Probe(s32,u32*)). port_pad_scan now probes 0..3 and merges via the pure port_pad_merge() helper (buttons ORed, larger-magnitude stick wins; wiimote/Classic stick folded in after the helper call since the helper signature carries only the GC stick).
- Input script gained an optional 5th column = port 1..4 (default 1); per-port current-line; scripted ports forced present in g_status; boot log now "input script loaded N lines ports=%x" (bit per port). Old 4-column scripts unchanged (t14d still reaches main_no=5); multi-port script ports=5 (ports 1 and 3).
- 21 (Claude): added WPAD_ScanPads() (was missing -> Wiimote buttons never refreshed) and Classic stick scaling (raw unsigned pos around center -> +-80).

## Task 22: reach 4P VS and 2P VS from title via input script
- Key repeat (joy.c 205-219): joycur bit set only when joycnt==1 (first frame of hold) OR joycnt>=joycur1 && (joycnt-joycur1)%joycur2==0. main_menu.c sets joycur1=0x18=24, joycur2=6. So a 6-frame hold registers EXACTLY ONCE -> each 6-frame DOWN press moves panel select by 1.
- Panel nav (menuMainPanel_input 2650): select = WrapI(0, count, select+vec); DOWN vec++ (U_JPAD vec--). _getKeyRep returns joycur[main_joy[contNo]] (held/repeat state).
- Title exit: START@300 (6f) port1 -> demo_title line 908 sets title_time=720 -> 60f fade -> main menu ~360, panel active ~380. First DOWN@420 safe.
- Main menu tbl_5517={MODE_PLAY1,MODE_PLAY2,MODE_PLAY4,MODE_OPTION} count4 select0. DOWNx2 -> select2=MODE_PLAY4. A confirm.
- MODE_PLAY4 panel table _play4_5342={ONE,TWO,THREE,FOUR_PLAYERS} count4; handler 5308: if(select<evs_playmax) mode=_mode_5570[select], evs_playcnt=4. _mode_5570={TYPE1..TYPE4}. Need select3=FOUR_PLAYERS -> DOWNx3 in this panel -> A -> MODE_PLAY4_TYPE4.
- MODE_PLAY4_TYPE4 panel (5590): table _play4type_5343={4_PLAYER_VS,4_TEAM_BATTLE,FLASH} count3; select0=4_PLAYER_VS -> _mode_5688[0]=MODE_PLAY4_CH. A confirm.
- 2P: main DOWNx1 -> select1=MODE_PLAY2. MODE_PLAY2 panel _play2_5341={2_PLAYER_VS,FLASH,SCORE_ATTACK} count3; select0=2_PLAYER_VS -> mode_5557[0]=MODE_VSMAN_NS -> _tblVM_5600[0][0]=MODE_VSMAN_CH. evs_playcnt=2, evs_gamesel=GSL_2PLAY.
- Name select menuNameSelPanel_input1 (2835) uses main_joy[i] (port i+1); depth0 start, A/START confirms; flow=1 when all confirmed; MODE_VSMAN_NS handler checks nameSel.flow>0.
- Char select menuChSel_inputMan (7512) uses playerTable[i] (port i+1); all depth==1 -> subDepth++; subDepth==2 starts game. A on each port.
- Logging edit dm_game_main.c line 7874 (after key_cntrol_init): PORT_LOG("game: start players=%d gamesel=%d", evs_playcnt, evs_gamesel).
- Scripts: in4p.txt START@300; main DOWN@420,DOWN@480; A@540; MODE_PLAY4 DOWN@600,DOWN@660,DOWN@720; A@780; TYPE4 A@840; char A@960 p1,1020 p2,1080 p3,1140 p4. in2p.txt START@300; DOWN@420; A@480; name A@600 p1,660 p2; char A@780 p1,840 p2.
- 22 (Claude): 2P and 4P VS verified at full speed (DR_PADS=4, scripts .strata/scratch/t22/in2p.txt / in4p.txt): both
  bottles/four bottles render, game reaches DRAW/Replay panel. Remaining VS cosmetics: dark boxes behind stars and part of
  "POINTS", black box right of the VIRUS LEVEL banner, stray dotted lines near the referee (same transparency family as 19b).
- Harness (Claude): dolphin-run.sh now deletes old framedump_*.png/last.png in <outdir> first (frames from older runs were
  mixed in and looked like a freeze). Dolphin runs at ~7 fps while Strata is generating (CPU 450% + GPU 100%): a test
  started next to a busy Strata looks frozen at ~frame 600 - pause Strata (kill -STOP/-CONT) or run after it finishes.

## 23b whole-image path (task 23b)
- Whole-image hint path in gfx_tex.c (gfx_tex_bind lines ~361-464 + unified texcoord rule in gfx_tex_draw_tri/gfx_tex_rect) is CORRECT: gate passes (t180=0.0 nb=0.80 sat=87 rows=0 textures=249 strips=5).
- Do NOT leave a `hint = NULL` perf-probe in the source; the real line is `const PortImgHint *hint = gfx_st.img_hint;`.
- sat=11.3 at frame 420 is NOT a bug: frame 420 is a grayscale title/menu moment and is pixel-identical to ref20 (diffmean 0.0). The gate's sat>60 check is on frame 1500 (gameplay), where sat=87-126.
- The gate needs framedump_1500.png; a 90s dolphin run only reaches it when Strata isn't hogging CPU (load ~8 -> ~420 frames in 90s). If the gate fails with FileNotFoundError on framedump_1500.png, it is the CPU-hog env blocker, not a code bug: re-run after Strata finishes, or verify with a 340s run (reaches ~20k frames).

## 24 texture wrap/mirror/clamp (task 24)
- cm is BIT FLAGS (G_TX_WRAP=0, G_TX_MIRROR=1, G_TX_CLAMP=2), not an enum. Old code `cms==0?CLAMP:cms==1?REPEAT:MIRROR` was wrong.
- Per-axis rule: clamp bit (cm&2) → GX_CLAMP + POT padding; else mask!=0 → GX_REPEAT (or GX_MIRROR if cm&1) with gx_size=2^mask; else mask==0 → GX_CLAMP.
- gfx_axis_mode (port/gfx/gfx_wrap.c) implements this; returns gx_mode (0/1/2) and gx_size.
- shift (shifts/shiftt): apply to 10.5 texcoord BEFORE masking: shift 1..10 → s>>shift, 11..15 → s<<(16-shift). gfx_tex_shift10 helper in gfx_tex.c.
- For wrap axes: tile the decoded RGBA8 to 2^mask texels (gfx_tile_rgba8) so GX_REPEAT wraps correctly; for clamp axes: POT-pad edge pixels.
- Cache key now folds cm/mask into crc so different wrap modes for same tile don't collide.
- Guard: gx_w*gx_h > 8192 → bail (tiled_buf is 8192*4 bytes).
- Hint path (23b) and alias path (18b): always GX_CLAMP, reset wrap/period/shift statics to 0.
- Tri texcoords: wrap → tx = shifted_10_5/32/period; clamp+shift → (shifted_10_5/32 - uls)/tw; clamp+shift0 → existing formula.
- Rect texcoords: wrap → s0 = shifted/32 (no uls); clamp+shift → shifted/32 - uls; clamp+shift0 → existing.
- Makefile.wii: added port/gfx/gfx_wrap.c to WII_SRCS.

## Reference renderer (Claude, 2026-10-21) - ground truth for pixel comparison
- /home/rhy/drmariowii/refemu: mupen64plus core (built from source, NO_ASM=1 DEBUGGER=1 DEBUGGER_NO_DISASM=1, local
  sysroot from unpacked -dev debs, env.sh) + rsp-cxd4 (LLE RSP) + angrylion-rdp-plus (cycle-accurate RDP) + our input
  plugin input-script/ (replays the port's input scripts, 5-column format, all 4 pads present).
- refemu/run-ref.sh <script|/dev/null> <frame list "30,34,..."> <outdir> [timeout]: writes ref_<frame>.png = the EXACT
  framebuffer the VI shows (read from RDRAM at VI_ORIGIN, 320x240, no VI filter/overscan). ~35x faster than real time.
- port side: DR_43=1 .strata/dolphin-run.sh ... (4:3, no widescreen squeeze); dumps are 640x528.
- port/tools/refcompare.py <port_dir> <ref_dir> <out> [--window N]: normalises both to 320x240, matches each port
  frame to the best reference frame within +-N frames (the port boots ~170 frames earlier), writes report.txt and
  cmp_<N>.png (port | reference | heat map) for the worst frames.
- (Claude 10-22) FIXED: osSetTime was a stub -> osGetTime counted from boot -> dm_game_graphic_onDoneSawp's
  13500-OS_CYCLES_TO_USEC(osGetTime()) wrapped -> ~30 min sleeps ("frozen picture"). osGetTime now counts from the
  last osSetTime. FIXED: 23b whole-image hash memo kept across tasks (stale texture when a new scene reused a heap
  address -> menu art as game background); reset per task (gfx_tex_task_begin), palette hash sizes 32/512 bytes.
- (Claude 10-22) 25b: W-sync works on port+reference; "only the latest completed segment plays" rule; 1p.txt mashes
  START until W 6. refcompare.py --port-log/--ref-log aligns by W events.
- OPEN (task 26): game logic runs once per 4 VI (15 fps). Not retraceMQ size (tested 8: same). gfx task ~4.6 ms.
- 26 (noted by Claude): the "15 fps logic" was a MEASUREMENT bug: port_cont_scan (and with it port_frame_now, the input
  script clock and controller polling) ran once per event-pump wake (~4 per VI: retrace+timer+SP+DP) instead of once per
  retrace. Fixed: scan once per VI event. Input-script frame numbers written before 10-22 01:07 were in 4x-fast units.

## Task 28: all-modes comparison data (2026-10-22)
- .strata/scratch/cmp/run_all.sh: per mode build+port(DR_43=1 DR_PADS=4,120s)+refemu(30..7200 step 3)+refcompare(--window 60 --worst 6).
- Dolphin's kill truncates the LAST framedump PNG (0-byte tail) -> refcompare crashed; run_all.sh now deletes unreadable PNGs before comparing.
- vs4p compared only 9 frames: the port's 4-pad vs4p run boots ~170 frames earlier and most port frames had no ref candidate in +-60 after event alignment (ref side waits). Data only; not fixed.
- Worst frame is the same early frame (16,32 40,16 24,32 blocks, diff 101.29) in most modes: title/attract mismatch, likely the boot-offset region.

## Task 29: 4P Vs heap-overflow investigation (2026-10-22)
- Goal: find whether the 4P Vs crash (zeroed thread context ~frame 286) is a heap overflow past Heap_bufferp.
- Heap_bufferp = 0x80205000-0x80124610 = 920048 bytes (buffer1.c). Added a SEPARATE `u8 heap_guard[4096] BSS;` array right after it (NOT an enlarged Heap_bufferp - enlarging it shifted gfx_freebuf and crashed at frame 10 because gfxInit(gfx_freebuf) at main.c:73 read the guard). Same TU keeps gfx_freebuf at its original offset.
- heapchk.c (port/wii/): port_heap_guard_init() fills guard 0xA5; port_heap_guard_check() (called from log.c flusher LWP every 1s) scans the guard and logs "heap: guard broken"/"heap: OVERFLOW" if scribbled; port_heap_note() (wired into BUFFER_* macros in buffers.h under TARGET_WII) tracks high-water. Do NOT include buffers.h in heapchk.c (libultra.h bcopy clash with devkitPPC string.h).
- RESULT: GATE passes (frames=237 >= 150, "heap: hw=" in log). Log shows "heap: hw=0 of 920048" at every scene change and NO "guard broken"/"OVERFLOW" lines even at the crash point (~frame 242). CONCLUSION: the heap never overflowed into the guard -> HEAP OVERFLOW IS RULED OUT as the crash cause. No heap enlargement needed.
- hw=0 because the BUFFER_* sites in dm_game_main.c are not the active heap path in 4P Vs; the guard scan is the comprehensive detector and it never fired.
- Next: the ~frame 242-286 crash is NOT heap overflow; look elsewhere (thread context zeroing, stack, or a bad pointer/segment).

## Task 29b: thread-context guards (2026-10-22)
- Goal: detect what zeroes a thread's saved context/stack. Added 64-byte 0x5A canaries at the LOW end of every LWP stack (game threads, pump, timer, RSP, log-flusher) + g_sched[] slot-field snapshots (t, entry, lwp, stack), checked every VI from the event pump.
- guard.c (port/wii/): port_guard_range (range registry), port_guard_canary (fill+snapshot 64B 0x5A), port_guard_boot_ranges (logs "guard: ranges %s %08x-%08x" per range), port_guard_check (LWP canary check, first mismatch logs "guard: %s canary broken (frame %u) at +%u exp=%02x got=%02x w0..w7"). No-op stubs under non-TARGET_WII.
- os_sched.c: snapshot arrays (g_guard_copy[MAX_THREADS][64], g_guard_t/entry/lwp/stack[], g_guard_frame, g_guard_broke); osCreateThread fills canary + snapshots slot fields + registers gthread/g_sched ranges once; port_guard_vi() increments frame, lazy-boot-ranges, calls port_guard_check, then checks each active slot's canary+fields, logs first mismatch with "guard: thread slot %d %s broken (frame %u, last running id %d)" + 8 words at &g_thread_stacks[i][0] or &g_sched[i].
- os_events.c/os_rsp.c/log.c: register pump/timer/RSP/log canary+range at their boot sites.
- GATE verified: "guard: ranges" present (5 range lines); "guard: thread slot .* broken" ABSENT; cmp_mode vs4p printed "median 39.11 worst 101.29 frames 237" (237 >= 150).
- RESULT: ran 3x vs4p + 3x vs2p (120 frames each): ALL NO_BROKEN. No guard fired in any run. The stack/slot canaries do NOT catch the zeroed-context writer -> the culprit is likely libogc-side (context saved/zeroed outside our LWP stacks/slots), not a stack-buffer overrun into our own buffers. Guards stay in as a tripwire.

## Task 30a: story/flash background (rtt alias) diagnostic (2026-10-22)
- Detection now WORKS: story cached log fires "rtt: offscreen cimg 0x80b0ef00 w=328" (genuinely off-screen, unlike 18b's in_fb 0x80d561ec). Flash fires at 0x80b7a840 w=328. Classic has NO rtt lines at all -> Classic draws its background straight to the framebuffer and never uses the off-screen snap buffer, so "Classic works with the same code" just means Classic never enters this path.
- REDRAW side WORKS: many "rtt: bindalias aaddr=... top=0/6/12...216 w=328" lines -> gfx_tex_bind binds the alias_obj for each 6-row strip of the snap buffer. So CopyTexBlock16/COPY redraw is fine (matches 18a test-pattern conclusion).
- CONTENT side is WRONG: "rtt: alias stats samp=19680 white=0 black=19680 color=0" in BOTH story and flash -> the GX_CopyTex(alias_buf) at gfx_xform.c:420 reads a BLACK EFB. The EFB does not hold the background at flush time.
- Flush timing: gfx_rtt_flush_if_active() runs at gfx.c:233 (end of the display-list task that SET the off-screen cimg) and also in xf_cimg's else-branch (gfx_xform.c:492) when cimg returns to the framebuffer. Only ONE gpu-alias line per run -> flush fired once, at the set-cimg task end, before the background was ever rasterized into the EFB.
- Most likely cause: the game fills the snap buffer with a TEXTURE COPY (CopyTexBlock16 / G_TEXRECT into the RAM buffer), not with rasterized draws to the EFB. The port's GX_CopyTex(EFB->alias) therefore reads a cleared/black EFB. The correct content is already in the snap-buffer RAM at gfx_resolve(rtt_n64); the alias should be built from that RAM, not from the EFB.
- Fresh-run caveat (NOT a regression): the DR_DUMPEFB=1 diagnostic run never reached "game: start" (stuck at main_no=2 after W6) because dumping 3810 EFB PNGs slows the emulator and desyncs the per-VI clock from story.txt's frame-numbered presses. The cached cmp logs (no EFB dumping) are the reliable signal. Do not trust DR_DUMPEFB runs for game progression.
- NEXT: fix the content side in port/gfx only. Read the COPY/TEXRECT handling in gfx_tex.c to see whether the snap-buffer RAM is filled by the game's copy; if so, build alias_buf from that RAM (convert RGBA5551 -> RGB565) instead of GX_CopyTex from EFB. Recheck Classic (no rtt) and Flash don't regress.

## Task 30b: story-bg copy width (2026-10-22)
- Fix applied in port/gfx/gfx_xform.c: clamp the GPU alias copy to the 640-wide EFB.
  `uint32_t copy_w = rtt_w*2u; if (copy_w > 640u) copy_w = 640u; alias_vis = copy_w/2u;`
  GX_SetTexCopySrc/Dst and GX_InitTexObj now use copy_w (was rtt_w*2 = 656, invalid on a 640-wide EFB).
  gfx_rtt_alias_lookup returns *w = alias_vis (=320 N64 cols) so texcoords >= 320 clamp to the last copied column.
  Comment updated in gfx_tex.c. Wii DOL builds clean.
- ENV BLOCKER (same as 18b/23b): every FRESH headless run dies ~29-46 emulator frames in with
  "MESA-EGL: DRI3 error: Could not get DRI3 device" then "The X11 connection broke (error 1). Did the X11 server die?".
  Confirmed NOT my edit: reverting the clamp to the old 656-wide behavior crashes IDENTICALLY (X11 dies ~4263 log lines /
  29 frames, no game: start). The cached task-30a story log (34636 lines, reached game: start) ran fine earlier, so the
  GL/Xvfb harness has regressed since then. No Xvfb processes leak (ps shows 0); stale harness-* dirs cleaned; /tmp/.X0-lock
  removed; still dies. The emulator's OGL backend cannot get a DRI3 device now -> software Mesa -> X tears down early.
- GATE cannot pass in this environment: cmp_mode.sh story prints "median 75.47 worst 101.29 frames 46" (only 46 frames
  before X dies; never reaches the stage). classic/flash not attempted (same blocker). The fix is correct in source but
  UNVERIFIED end-to-end because the harness cannot run the game to the snapshot moment.
- NEED owner guidance: restore GL/DRI3 for the flatpak Dolphin OGL backend (or a GFXBackend that survives Xvfb), then
  re-run the GATE. Do not keep re-running the identical cmp command - it is the harness, not the code, that fails.
