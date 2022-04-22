#include "audio.h"

#include <math.h>

void audioCallback(int16_t *buffer, unsigned size)
{
    return;  // TODO: remove this

    static float x = 0.0f;

    for (unsigned i = 0; i < size * 2; i += 2) {
        int16_t sample = sinf(x) * INT16_MAX * 0.25f;
        buffer[i + 0] = sample;
        buffer[i + 1] = 0;
        x += 2000.0f / AUDIO_SAMPLES_PER_SECOND;
    }

    if (x > AUDIO_SAMPLES_PER_SECOND)
        x -= (float)AUDIO_SAMPLES_PER_SECOND;
}


void initAudio(void)
{
    AudioInfo info = {
        .writeCallback = audioCallback,
    };

    initAudioEngine(info);
}


void exitAudio(void)
{
    exitAudioEngine();
}
