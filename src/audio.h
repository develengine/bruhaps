#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>

#define AUDIO_SAMPLES_PER_SECOND 48000

typedef void (*AudioWriteCallback)(int16_t *buffer, unsigned size);

void initAudio(AudioWriteCallback wc);
void exitAudio(void);

#endif