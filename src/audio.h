#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>

#define AUDIO_SAMPLES_PER_SECOND 48000

typedef void (*AudioWriteCallback)(int16_t *buffer, unsigned size);

typedef struct
{
    AudioWriteCallback writeCallback;
} AudioInfo;

void initAudioEngine(AudioInfo info);
void exitAudioEngine(void);

void initAudio(void);
void exitAudio(void);

#endif
