/*
 * Asciiverse terminal edition built without sound (make SOUND=0): every
 * sound call does nothing.
 *
 * Copyright (C) 2026 David Sandquist. GNU Library GPL v2 or later, as the game.
 */
static int audio_dummy;

extern "C" {
int   ghack_mix_open_audio(int, unsigned short, int, int) { return 0; }
void  ghack_mix_close_audio(void) {}
void *ghack_mix_load_wav(const char *) { return &audio_dummy; }
void *ghack_mix_load_mus(const char *) { return &audio_dummy; }
void  ghack_mix_free_music(void *) {}
int   ghack_mix_play_channel(int, void *, int) { return 0; }
int   ghack_mix_halt_channel(int) { return 0; }
int   ghack_mix_volume_chunk(void *, int) { return 0; }
int   ghack_mix_volume_music(int) { return 0; }
int   ghack_mix_fade_in_music(void *, int, int) { return 0; }
int   ghack_mix_halt_music(void) { return 0; }
}
