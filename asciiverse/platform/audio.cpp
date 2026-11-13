/*
 * Asciiverse terminal edition: the game's sound through SDL2_mixer, with no
 * video at all. This is the only file that includes the real SDL2 headers;
 * the game itself sees the SDL 1.2 shim (include/ghack_mixer_shim.h).
 *
 * --silent, or GOBLINHACK_SILENT set, opens no audio device at all.
 *
 * Copyright (C) 2026 David Sandquist. GNU Library GPL v2 or later, as the game.
 */
#include <SDL.h>
#include <SDL_mixer.h>

extern bool terminal_silent(void);

static bool audio_on;
static int  audio_dummy;

extern "C" {

int ghack_mix_open_audio(int frequency, Uint16 format, int channels, int chunksize)
{
    if (terminal_silent()) {
        return 0;
    }
    if (SDL_Init(SDL_INIT_AUDIO) != 0) {
        return -1;
    }
    if (Mix_OpenAudio(frequency, format, channels, chunksize) != 0) {
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        return -1;
    }
    Mix_AllocateChannels(32);
    audio_on = true;
    return 0;
}

void ghack_mix_close_audio(void)
{
    if (audio_on) {
        Mix_CloseAudio();
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        audio_on = false;
    }
}

/* without sound, every load gives a token the game keeps but never plays */
void *ghack_mix_load_wav(const char *file)
{
    if (!audio_on) {
        return &audio_dummy;
    }
    return Mix_LoadWAV(file);
}

void *ghack_mix_load_mus(const char *file)
{
    if (!audio_on) {
        return &audio_dummy;
    }
    return Mix_LoadMUS(file);
}

void ghack_mix_free_music(void *m)
{
    if (audio_on && m && m != &audio_dummy) {
        Mix_FreeMusic((Mix_Music *) m);
    }
}

int ghack_mix_play_channel(int channel, void *chunk, int loops)
{
    if (!audio_on || chunk == &audio_dummy) {
        return 0;
    }
    return Mix_PlayChannel(channel, (Mix_Chunk *) chunk, loops);
}

int ghack_mix_halt_channel(int channel)
{
    return audio_on ? Mix_HaltChannel(channel) : 0;
}

int ghack_mix_volume_chunk(void *chunk, int volume)
{
    if (!audio_on || chunk == &audio_dummy) {
        return 0;
    }
    return Mix_VolumeChunk((Mix_Chunk *) chunk, volume);
}

int ghack_mix_volume_music(int volume)
{
    return audio_on ? Mix_VolumeMusic(volume) : 0;
}

int ghack_mix_fade_in_music(void *music, int loops, int ms)
{
    if (!audio_on || music == &audio_dummy) {
        return 0;
    }
    return Mix_FadeInMusic((Mix_Music *) music, loops, ms);
}

int ghack_mix_halt_music(void)
{
    return audio_on ? Mix_HaltMusic() : 0;
}

}
