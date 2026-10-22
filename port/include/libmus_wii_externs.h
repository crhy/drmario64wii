#ifndef LIBMUS_WII_EXTERNS_H
#define LIBMUS_WII_EXTERNS_H
/* On N64, player_commands.c / player_api.c / player_fifo.c are #included
 * directly into player.c, so they see its statics. On Wii they are
 * compiled standalone; declare those names here. */
extern ALPlayer plr_player;
extern int max_channels;
extern ALVoice *mus_voices;
extern channel_t *mus_channels;
extern channel_t *mus_channels2;
extern int mus_vsyncs_per_second;
extern ALMicroTime mus_next_frame_time;
extern unsigned short mus_master_volume_effects;
extern unsigned short mus_master_volume_songs;
extern unsigned long mus_current_handle;
extern long mus_random_seed;
extern ptr_bank_t *mus_init_bank;
extern ptr_bank_t *mus_default_bank;
extern musBool mus_songfxchange_flag;
extern int mus_last_fxtype;
extern fx_header_t *libmus_fxheader_current;
extern fx_header_t *libmus_fxheader_single;
extern LIBMUScb_marker marker_callback;
extern unsigned long __muscontrol_flag;
extern command_func_t jumptable[];
extern ALMicroTime __MusIntMain(void *node);
extern void __MusIntProcessContinuousVolume(channel_t *cp);
extern void __MusIntProcessContinuousPitchBend(channel_t *cp);
extern void __MusIntInitialiseChannel(channel_t *cp);
extern void __MusIntRemapPtrBank(char *pptr, char *wptr);
extern void __MusIntHandleSetFlag(unsigned long handle, unsigned long clear, unsigned long set);
extern int __MusIntRandom(int range);
extern unsigned long __MusIntStartEffect(channel_t *cp, fx_header_t *header, int number, int volume, int pan, int priority);
extern unsigned long __MusIntFindChannelAndStart(fx_header_t *header, int number, int volume, int pan, int priority);
extern musHandle __MusIntStartSong(void *addr);
#endif
