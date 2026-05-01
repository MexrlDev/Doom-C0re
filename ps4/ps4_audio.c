#include "core.h"
#include "doomgeneric.h"

extern void *G;
extern void *D;
extern s32 audio_h;
extern void *aud_out_fn;

#define SAMPLE_RATE     48000
#define SAMPLES_PER_BUF 256

static int16_t mix_buffer[SAMPLES_PER_BUF * 2]; // stereo interleaved

void I_InitSound(void) {
    // Audio handle already opened in main.c
}

void I_SubmitSound(void) {
    if (audio_h < 0 || !aud_out_fn) return;

    // DoomGeneric prepares sound data in the `sndOutput` array (16-bit mono, 11025 Hz).

    extern void *sndOutput;   // pointer to mixed audio buffer (from Doom)
    extern int sndSamples;    // number of samples available

    if (sndSamples <= 0) return;

    int16_t *src = (int16_t *)sndOutput;
    int in_len = sndSamples;
    int out_len = SAMPLES_PER_BUF;

    // Linear interpolation ratio
    for (int i = 0; i < out_len; i++) {
        float pos = (float)i * in_len / out_len;
        int idx = (int)pos;
        float frac = pos - idx;
        int next = (idx + 1 < in_len) ? idx + 1 : idx;
        int16_t val = (int16_t)(src[idx] * (1.f - frac) + src[next] * frac);
        mix_buffer[i * 2] = val;
        mix_buffer[i * 2 + 1] = val;
    }

    // Send to hardware
    NC(G, aud_out_fn, (u64)audio_h, (u64)mix_buffer, 0, 0, 0, 0);
    // Note: sceAudioOutOutput blocks until the buffer is consumed.
}

void I_ShutdownSound(void) {
    // audio handle closed in cleanup
}
