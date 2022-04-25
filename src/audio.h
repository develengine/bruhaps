#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>
#include <assert.h>
#include <stdbool.h>

#define AUDIO_SAMPLES_PER_SECOND 48000

typedef void (*AudioWriteCallback)(int16_t *buffer, unsigned size);

typedef struct
{
    AudioWriteCallback writeCallback;
} AudioInfo;

#define MAX_SOUND_COUNT 32

typedef struct
{
    uint8_t *data;
    unsigned playing;   /* true/false */
    unsigned times;     /* 0 is indefinitely */
    unsigned start, end;
    float volumeL, volumeR;
} SoundInfo;

typedef struct
{
    int next;
    SoundInfo info;
    int padding[22];
} Sound;

// FIXME: may need to be adjusted (adjustable)
static_assert(sizeof(Sound) == 128, "Sound not cacheline aligned");


void initAudioEngine(AudioInfo info);
void exitAudioEngine(void);

void initAudio(void);
void exitAudio(void);

bool playSound(SoundInfo info, volatile SoundInfo **handle);

int16_t *loadWAV(const char *path, unsigned *length);

#endif
