/*
 * Asciiverse terminal edition: the SDL_mixer calls Goblin Hack makes, sent
 * to SDL2_mixer by asciiverse/platform/audio.cpp (the only file that sees the
 * real SDL2 headers), or silent when built without sound or run with
 * --silent / GHACK_SILENT.
 *
 * Copyright (C) 2026 David Sandquist. GNU Library GPL v2 or later, as the game.
 */
#ifndef GHACK_MIXER_SHIM_H
#define GHACK_MIXER_SHIM_H

#include "ghack_sdl_shim.h"

typedef struct Mix_Chunk Mix_Chunk;
typedef struct _Mix_Music Mix_Music;

#define MIX_DEFAULT_FORMAT 0x8010

#ifdef __cplusplus
extern "C" {
#endif
int   ghack_mix_open_audio(int frequency, unsigned short format, int channels, int chunksize);
void  ghack_mix_close_audio(void);
void *ghack_mix_load_wav(const char *file);
void *ghack_mix_load_mus(const char *file);
void  ghack_mix_free_music(void *music);
int   ghack_mix_play_channel(int channel, void *chunk, int loops);
int   ghack_mix_halt_channel(int channel);
int   ghack_mix_volume_chunk(void *chunk, int volume);
int   ghack_mix_volume_music(int volume);
int   ghack_mix_fade_in_music(void *music, int loops, int ms);
int   ghack_mix_halt_music(void);
#ifdef __cplusplus
}
#endif

#define Mix_OpenAudio   ghack_mix_open_audio
#define Mix_CloseAudio  ghack_mix_close_audio
#define Mix_LoadWAV(f)  ((Mix_Chunk *) ghack_mix_load_wav(f))
#define Mix_LoadMUS(f)  ((Mix_Music *) ghack_mix_load_mus(f))
#define Mix_FreeMusic   ghack_mix_free_music
#define Mix_PlayChannel ghack_mix_play_channel
#define Mix_HaltChannel ghack_mix_halt_channel
#define Mix_VolumeChunk ghack_mix_volume_chunk
#define Mix_VolumeMusic ghack_mix_volume_music
#define Mix_FadeInMusic ghack_mix_fade_in_music
#define Mix_HaltMusic   ghack_mix_halt_music

#endif
